import hashlib
import posixpath
import struct
import time

from protocol import build_packet, MSG_LIBRARY_ENTRY
from uart_writer_client import send_packet_locked
from mpd_client import mpd_command, mpd_lsinfo, mpd_search_with_tags, _mpd_escape

LIB_BROWSE_UP = 0xFFFE
LIB_BROWSE_ROOT = 0xFFFF
LIBRARY_ENTRY_FOLDER = 0
LIBRARY_ENTRY_TRACK = 1
LIBRARY_ENTRY_EMPTY = 2  # sentinel for a zero-entry folder
LIBRARY_ENTRY_RADIO = 3
MAX_LIBRARY_ENTRIES = 200  # cap per directory listing, not the whole library
MAX_FOLDER_TRACKS = 50
MAX_LIBRARY_NAME_LEN = 55
MAX_LIBRARY_ALBUM_LEN = 40
# MD5 hex digest length, used as the moOde thmcache lookup key.
MAX_LIBRARY_ART_HASH_LEN = 32
RADIO_DIRECTORY = "RADIO"
OSDISK_DIRECTORY = "OSDISK"  # internal storage mount, hidden from the Library browser
SAVED_PLAYLISTS_NAME = "Saved Playlists"
# Synthetic browse_path used to mark the virtual Saved Playlists folder — never a real MPD path.
SAVED_PLAYLISTS_TOKEN = "\x00saved-playlists\x00"


class _BrowseState:
    def __init__(self):
        self.browse_path = ""       # "" == library root
        self.browse_entries = []    # [(is_directory, full_path), ...] for the last listing sent
        self.radio_browse = False
        self.playlist_entries = []  # full paths for the last playlist listing sent
        self.search_entries = []    # full paths for the last search-result listing sent
        self.library_seq = 0

_state = _BrowseState()


def set_radio_browse(enabled):
    """Select the saved Moode stations folder or the regular MPD music library."""
    _state.radio_browse = enabled
    _state.browse_path = ""
    _state.browse_entries = []


def _list_saved_playlist_names():
    try:
        return [
            line[len("playlist: "):]
            for line in mpd_command("listplaylists")
            if line.startswith("playlist: ")
        ]
    except Exception as e:
        print(f"⚠️ MPD listplaylists failed: {e}", flush=True)
        return []


def _root_entries():
    """Root listing with OSDISK hidden and saved playlists collapsed into one virtual folder."""
    raw_entries = mpd_lsinfo("")
    playlist_names = set(_list_saved_playlist_names())
    filtered = [
        (is_dir, full_path) for is_dir, full_path in raw_entries
        if not (is_dir and posixpath.basename(full_path) == OSDISK_DIRECTORY)
        and not (not is_dir and full_path in playlist_names)
    ]
    if playlist_names:
        filtered.append((True, SAVED_PLAYLISTS_TOKEN))
    return filtered


def send_library_entry(index, total, entry_type, name, album="", art_hash=""):
    name_bytes = name.encode("utf-8")[:MAX_LIBRARY_NAME_LEN]
    album_bytes = album.encode("utf-8")[:MAX_LIBRARY_ALBUM_LEN]
    # art_hash must be a plain MD5 hex digest (or empty) — never send anything else onto the wire.
    assert len(art_hash) in (0, MAX_LIBRARY_ART_HASH_LEN), f"Invalid art_hash length: {art_hash!r}"
    hash_bytes = art_hash.encode("ascii")
    payload = (
        bytes([entry_type]) + struct.pack("<HH", index, total)
        + bytes([len(name_bytes)]) + name_bytes
        + bytes([len(album_bytes)]) + album_bytes
        + bytes([len(hash_bytes)]) + hash_bytes
    )
    send_packet_locked(build_packet(MSG_LIBRARY_ENTRY, _state.library_seq, 0, payload))
    _state.library_seq = (_state.library_seq + 1) & 0xFF


def handle_browse_request(params):
    target = params[0]

    if target == LIB_BROWSE_ROOT:
        _state.browse_path = ""
    elif _state.radio_browse:
        print(f"Invalid radio browse target {target}", flush=True)
        return
    elif target == LIB_BROWSE_UP:
        _state.browse_path = "" if _state.browse_path == SAVED_PLAYLISTS_TOKEN else posixpath.dirname(_state.browse_path)
    elif 0 <= target < len(_state.browse_entries) and _state.browse_entries[target][0]:
        _state.browse_path = _state.browse_entries[target][1]
    else:
        print(f"⚠️ Invalid browse target {target} (have {len(_state.browse_entries)} entries)", flush=True)
        return

    try:
        if _state.radio_browse:
            entries = mpd_lsinfo(RADIO_DIRECTORY)
        elif _state.browse_path == SAVED_PLAYLISTS_TOKEN:
            entries = [(False, name) for name in _list_saved_playlist_names()]
        elif _state.browse_path == "":
            entries = _root_entries()
        else:
            entries = mpd_lsinfo(_state.browse_path)
    except Exception as e:
        print(f"⚠️ MPD lsinfo failed: {e}", flush=True)
        entries = []

    _state.browse_entries = entries[:MAX_LIBRARY_ENTRIES]
    total = len(_state.browse_entries)
    location = RADIO_DIRECTORY if _state.radio_browse else (_state.browse_path or "(root)")
    print(f"Browse → {location} ({total} entries)", flush=True)

    if total == 0:
        send_library_entry(0, 0, LIBRARY_ENTRY_EMPTY, "")
        return

    for index, (is_dir, full_path) in enumerate(_state.browse_entries):
        if full_path == SAVED_PLAYLISTS_TOKEN:
            name = SAVED_PLAYLISTS_NAME
        else:
            name = posixpath.basename(full_path) or full_path
            if _state.radio_browse and name.endswith(".pls"):
                name = name[:-4]
        if is_dir:
            entry_type = LIBRARY_ENTRY_FOLDER
        elif _state.radio_browse or full_path.startswith(f"{RADIO_DIRECTORY}/"):
            entry_type = LIBRARY_ENTRY_RADIO
        else:
            entry_type = LIBRARY_ENTRY_TRACK
        send_library_entry(index, total, entry_type, name)
        time.sleep(0.008)  # pace sends so the ESP32 RX/BLE-notify pipeline can keep up


def handle_add_track(params):
    index = params[0]
    if not (0 <= index < len(_state.browse_entries)):
        print(f"⚠️ Invalid add-track index {index}", flush=True)
        return

    is_dir, full_path = _state.browse_entries[index]
    if is_dir:
        print(f"⚠️ Add-track index {index} is a folder, ignoring", flush=True)
        return

    try:
        if (_state.radio_browse
                or _state.browse_path == SAVED_PLAYLISTS_TOKEN
                or full_path.startswith(f"{RADIO_DIRECTORY}/")
                or full_path.endswith((".pls", ".m3u", ".m3u8", ".cue", ".asx"))):
            try:
                mpd_command(f'load "{_mpd_escape(full_path)}"')
            except Exception:
                mpd_command(f'add "{_mpd_escape(full_path)}"')
        else:
            try:
                mpd_command(f'add "{_mpd_escape(full_path)}"')
            except Exception:
                mpd_command(f'load "{_mpd_escape(full_path)}"')
        print(f"Added to queue: {full_path}", flush=True)
    except Exception as e:
        print(f"⚠️ MPD add/load failed: {e}", flush=True)


def _folder_tracks(folder_path, limit=MAX_FOLDER_TRACKS):
    """Return tracks below a folder, in MPD listing order, up to limit."""
    tracks = []
    pending = [folder_path]
    while pending and len(tracks) < limit:
        current_path = pending.pop(0)
        try:
            entries = mpd_lsinfo(current_path)
        except Exception as e:
            print(f"⚠️ MPD lsinfo failed for {current_path}: {e}", flush=True)
            continue
        for is_dir, full_path in entries:
            if is_dir:
                pending.append(full_path)
            else:
                tracks.append(full_path)
                if len(tracks) >= limit:
                    break
    return tracks


def _folder_from_current_listing(params):
    index = params[0]
    if not (0 <= index < len(_state.browse_entries)):
        print(f"⚠️ Invalid folder index {index}", flush=True)
        return None
    is_dir, full_path = _state.browse_entries[index]
    if not is_dir:
        print(f"⚠️ Folder index {index} is a track, ignoring", flush=True)
        return None
    return full_path


def handle_add_folder(params):
    folder_path = _folder_from_current_listing(params)
    if folder_path is None:
        return
    tracks = _folder_tracks(folder_path)
    try:
        for track in tracks:
            mpd_command(f'add "{_mpd_escape(track)}"')
        print(f"Added {len(tracks)} tracks from folder: {folder_path}", flush=True)
    except Exception as e:
        print(f"⚠️ Add-folder failed: {e}", flush=True)


def handle_replace_with_folder(params):
    folder_path = _folder_from_current_listing(params)
    if folder_path is None:
        return
    tracks = _folder_tracks(folder_path)
    try:
        mpd_command("clear")
        for track in tracks:
            mpd_command(f'add "{_mpd_escape(track)}"')
        print(f"Replaced playlist with {len(tracks)} tracks from folder: {folder_path}", flush=True)
    except Exception as e:
        print(f"⚠️ Replace-folder failed: {e}", flush=True)


def handle_playlist_request(_params):
    try:
        tracks = [
            line[len("file: "):]
            for line in mpd_command("playlistinfo")
            if line.startswith("file: ")
        ][:MAX_LIBRARY_ENTRIES]
    except Exception as e:
        print(f"⚠️ MPD playlistinfo failed: {e}", flush=True)
        tracks = []

    _state.playlist_entries = tracks
    total = len(tracks)
    print(f"Playlist → {total} tracks", flush=True)
    if total == 0:
        send_library_entry(0, 0, LIBRARY_ENTRY_EMPTY, "")
        return

    for index, full_path in enumerate(tracks):
        name = posixpath.basename(full_path) or full_path
        entry_type = LIBRARY_ENTRY_RADIO if full_path.startswith(f"{RADIO_DIRECTORY}/") else LIBRARY_ENTRY_TRACK
        send_library_entry(index, total, entry_type, name)
        time.sleep(0.008)


def handle_clear_queue(_params):
    try:
        mpd_command("clear")
        print("Queue cleared", flush=True)
    except Exception as e:
        print(f"⚠️ MPD clear failed: {e}", flush=True)


def handle_play_track(params):
    index = params[0]
    if not (0 <= index < len(_state.playlist_entries)):
        print(f"⚠️ Invalid play-track index {index}", flush=True)
        return

    try:
        mpd_command(f"play {index}")
        print(f"Playing queue position {index}: {_state.playlist_entries[index]}", flush=True)
    except Exception as e:
        print(f"⚠️ MPD play failed: {e}", flush=True)


def handle_remove_track(params):
    index = params[0]
    if not (0 <= index < len(_state.playlist_entries)):
        print(f"⚠️ Invalid remove-track index {index}", flush=True)
        return

    try:
        mpd_command(f"delete {index}")
        print(f"Removed queue position {index}: {_state.playlist_entries[index]}", flush=True)
    except Exception as e:
        print(f"⚠️ MPD delete failed: {e}", flush=True)


def _run_search(field, text):
    """Runs an MPD search and streams the results via MSG_LIBRARY_ENTRY, kept in
    _state.search_entries (separate from browse_entries, so browsing an actual
    folder can't be corrupted by a search or vice versa). Results are sorted by
    (album, track number, path) so the Android app can group consecutive same-
    album entries into album sections in correct track order without needing a
    separate track-number field on the wire."""
    text = text.strip()
    if not text:
        print("⚠️ Empty search text, ignoring", flush=True)
        return

    try:
        results = mpd_search_with_tags(field, text)[:MAX_LIBRARY_ENTRIES]
    except Exception as e:
        print(f"⚠️ MPD search failed: {e}", flush=True)
        results = []

    results.sort(key=lambda r: (r[1].lower(), r[2], r[0]))

    _state.search_entries = [path for path, _album, _track_num in results]
    total = len(results)
    print(f"Search[{field}]='{text}' → {total} results", flush=True)
    if total == 0:
        send_library_entry(0, 0, LIBRARY_ENTRY_EMPTY, "")
        return

    for index, (full_path, album, _track_num) in enumerate(results):
        name = posixpath.basename(full_path) or full_path
        album_dir = posixpath.dirname(full_path)
        art_hash = hashlib.md5(album_dir.encode("utf-8")).hexdigest() if album_dir else ""
        send_library_entry(index, total, LIBRARY_ENTRY_TRACK, name, album, art_hash)
        time.sleep(0.008)


def handle_library_search_artist(text):
    _run_search("artist", text)


def handle_library_search_album(text):
    _run_search("album", text)


def handle_library_search_any(text):
    _run_search("any", text)


def handle_add_search_result(params):
    index = params[0]
    if not (0 <= index < len(_state.search_entries)):
        print(f"⚠️ Invalid add-search-result index {index}", flush=True)
        return

    full_path = _state.search_entries[index]
    try:
        if (full_path.startswith(f"{RADIO_DIRECTORY}/")
                or full_path.endswith((".pls", ".m3u", ".m3u8", ".cue", ".asx"))):
            try:
                mpd_command(f'load "{_mpd_escape(full_path)}"')
            except Exception:
                mpd_command(f'add "{_mpd_escape(full_path)}"')
        else:
            try:
                mpd_command(f'add "{_mpd_escape(full_path)}"')
            except Exception:
                mpd_command(f'load "{_mpd_escape(full_path)}"')
        print(f"Added to queue from search: {full_path}", flush=True)
    except Exception as e:
        print(f"⚠️ MPD add/load failed: {e}", flush=True)
