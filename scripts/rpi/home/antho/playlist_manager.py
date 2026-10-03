import time

from library_browser import send_library_entry, LIBRARY_ENTRY_TRACK, LIBRARY_ENTRY_EMPTY, MAX_LIBRARY_ENTRIES
from mpd_client import mpd_command, _mpd_escape
from protocol import build_packet, MSG_PLAYLIST_RESULT
from uart_writer_client import send_packet_locked

MAX_PLAYLIST_NAME_LEN = 55
_result_seq = 0


def _send_playlist_result(ok, message=""):
    global _result_seq
    payload = bytes([1 if ok else 0]) + message[:MAX_PLAYLIST_NAME_LEN].encode("utf-8")
    send_packet_locked(build_packet(MSG_PLAYLIST_RESULT, _result_seq, 0, payload))
    _result_seq = (_result_seq + 1) & 0xFF


def _list_playlist_names():
    return [
        line[len("playlist: "):]
        for line in mpd_command("listplaylists")
        if line.startswith("playlist: ")
    ][:MAX_LIBRARY_ENTRIES]


def handle_playlist_list_request(_params):
    try:
        names = _list_playlist_names()
    except Exception as e:
        print(f"⚠️ MPD listplaylists failed: {e}", flush=True)
        _send_playlist_result(False, "Failed to fetch playlists")
        return

    total = len(names)
    print(f"Playlists → {total} found", flush=True)
    if total == 0:
        send_library_entry(0, 0, LIBRARY_ENTRY_EMPTY, "")
        return

    for index, name in enumerate(names):
        send_library_entry(index, total, LIBRARY_ENTRY_TRACK, name)
        time.sleep(0.008)  # pace sends, matches library_browser's own listing pacing


def handle_playlist_save(name):
    name = name.strip()
    try:
        mpd_command(f'save "{_mpd_escape(name)}"')
        print(f"Playlist saved: {name}", flush=True)
        _send_playlist_result(True, name)
    except Exception as e:
        print(f"⚠️ Playlist save failed: {e}", flush=True)
        _send_playlist_result(False, str(e))


def handle_playlist_save_overwrite(name):
    name = name.strip()
    try:
        mpd_command(f'rm "{_mpd_escape(name)}"')
    except Exception as e:
        print(f"⚠️ Playlist rm (pre-overwrite) failed: {e}", flush=True)
    try:
        mpd_command(f'save "{_mpd_escape(name)}"')
        print(f"Playlist overwritten: {name}", flush=True)
        _send_playlist_result(True, name)
    except Exception as e:
        print(f"⚠️ Playlist overwrite-save failed: {e}", flush=True)
        _send_playlist_result(False, str(e))


def handle_playlist_load(name):
    name = name.strip()
    try:
        mpd_command("clear")
        mpd_command(f'load "{_mpd_escape(name)}"')
        print(f"Playlist loaded: {name}", flush=True)
        _send_playlist_result(True, name)
    except Exception as e:
        print(f"⚠️ Playlist load failed: {e}", flush=True)
        _send_playlist_result(False, str(e))


def handle_playlist_delete(name):
    name = name.strip()
    try:
        mpd_command(f'rm "{_mpd_escape(name)}"')
        print(f"Playlist deleted: {name}", flush=True)
        _send_playlist_result(True, name)
    except Exception as e:
        print(f"⚠️ Playlist delete failed: {e}", flush=True)
        _send_playlist_result(False, str(e))
