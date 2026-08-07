import posixpath
import struct
import time

from protocol import build_packet, MSG_LIBRARY_ENTRY
from uart_writer_client import send_packet_locked
from mpd_client import mpd_command, mpd_lsinfo, _mpd_escape

LIB_BROWSE_UP = 0xFFFE
LIB_BROWSE_ROOT = 0xFFFF
LIBRARY_ENTRY_FOLDER = 0
LIBRARY_ENTRY_TRACK = 1
LIBRARY_ENTRY_EMPTY = 2  # sentinel for a zero-entry folder
MAX_LIBRARY_ENTRIES = 200  # cap per directory listing, not the whole library
MAX_LIBRARY_NAME_LEN = 55


class _BrowseState:
    def __init__(self):
        self.browse_path = ""       # "" == library root
        self.browse_entries = []    # [(is_directory, full_path), ...] for the last listing sent
        self.library_seq = 0

_state = _BrowseState()


def send_library_entry(index, total, entry_type, name):
    payload = bytes([entry_type]) + struct.pack("<HH", index, total) + name.encode("utf-8")[:MAX_LIBRARY_NAME_LEN]
    send_packet_locked(build_packet(MSG_LIBRARY_ENTRY, _state.library_seq, 0, payload))
    _state.library_seq = (_state.library_seq + 1) & 0xFF


def handle_browse_request(params):
    target = params[0]

    if target == LIB_BROWSE_ROOT:
        _state.browse_path = ""
    elif target == LIB_BROWSE_UP:
        _state.browse_path = posixpath.dirname(_state.browse_path)
    elif 0 <= target < len(_state.browse_entries) and _state.browse_entries[target][0]:
        _state.browse_path = _state.browse_entries[target][1]
    else:
        print(f"⚠️ Invalid browse target {target} (have {len(_state.browse_entries)} entries)", flush=True)
        return

    try:
        entries = mpd_lsinfo(_state.browse_path)
    except Exception as e:
        print(f"⚠️ MPD lsinfo failed: {e}", flush=True)
        entries = []

    _state.browse_entries = entries[:MAX_LIBRARY_ENTRIES]
    total = len(_state.browse_entries)
    print(f"Browse → {_state.browse_path or '(root)'} ({total} entries)", flush=True)

    if total == 0:
        send_library_entry(0, 0, LIBRARY_ENTRY_EMPTY, "")
        return

    for index, (is_dir, full_path) in enumerate(_state.browse_entries):
        name = posixpath.basename(full_path) or full_path
        entry_type = LIBRARY_ENTRY_FOLDER if is_dir else LIBRARY_ENTRY_TRACK
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
        mpd_command(f'add "{_mpd_escape(full_path)}"')
        print(f"Added to queue: {full_path}", flush=True)
    except Exception as e:
        print(f"⚠️ MPD add failed: {e}", flush=True)
