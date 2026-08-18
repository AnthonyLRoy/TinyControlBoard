import sys
import os
import struct
from types import ModuleType
import importlib

# 1. Prepare environment
sys.path.insert(0, os.path.abspath("scripts/rpi/home/antho"))

# Mock modules for Phase 1
import_failures = []
assertions_passed = []

print("=== RUNNING BEHAVIOR TESTS ===")

# --- Phase 1: Test library_browser.py ---
sent_packets = []
mpd_lsinfo_calls = []

mpd_mock_responses = {
    "RADIO": [(False, "RADIO/KEXP.pls")],
    "": [(False, "Music/Track.flac")]
}

def fake_build_packet(msg_type, seq, cmd_id, payload=b''):
    return (msg_type, seq, cmd_id, payload)

def fake_send_packet_locked(pkt):
    sent_packets.append(pkt)

def fake_mpd_lsinfo(path):
    print(f"Fake mpd_lsinfo called with: {path!r}")
    mpd_lsinfo_calls.append(path)
    return mpd_mock_responses.get(path, [])

def fake__mpd_escape(path):
    return path.replace("\\", "\\\\").replace('"', '\\"')

def fake_mpd_command(command_line):
    return []

# Inject mock modules for Phase 1
fake_protocol = ModuleType("protocol")
fake_protocol.MSG_LIBRARY_ENTRY = 0x07
fake_protocol.build_packet = fake_build_packet
sys.modules["protocol"] = fake_protocol

fake_uart = ModuleType("uart_writer_client")
fake_uart.send_packet_locked = fake_send_packet_locked
sys.modules["uart_writer_client"] = fake_uart

fake_mpd = ModuleType("mpd_client")
fake_mpd.mpd_lsinfo = fake_mpd_lsinfo
fake_mpd._mpd_escape = fake__mpd_escape
fake_mpd.mpd_command = fake_mpd_command
sys.modules["mpd_client"] = fake_mpd

try:
    import library_browser as lb
except Exception as e:
    print(f"Failed to import library_browser: {e}")
    sys.exit(1)

# Run library_browser.py assertions
try:
    print("\n--- Testing library_browser: SET_RADIO_BROWSE(True) ---")
    lb.set_radio_browse(True)
    lb.handle_browse_request([0xFFFF]) # LIB_BROWSE_ROOT

    # Assert mpd_lsinfo got "RADIO"
    assert "RADIO" in mpd_lsinfo_calls, f"Expected mpd_lsinfo_calls to contain 'RADIO', got {mpd_lsinfo_calls}"
    print("Assertion passed: mpd_lsinfo got 'RADIO'")
    assertions_passed.append("library_browser_radio_browse_path")

    # Assert KEXP.pls entry is sent with KEXP
    assert len(sent_packets) > 0, "No packets sent"
    found_kexp = False
    for pkt in sent_packets:
        msg_type, seq, cmd_id, payload = pkt
        if msg_type == fake_protocol.MSG_LIBRARY_ENTRY:
            entry_type = payload[0]
            index, total = struct.unpack("<HH", payload[1:5])
            name = payload[5:].decode("utf-8")
            print(f"Sent Entry: type={entry_type}, index={index}, total={total}, name={name!r}")
            if name == "KEXP":
                found_kexp = True
    assert found_kexp, f"Expected display name 'KEXP', but it was not found in sent packets. Sent packets: {sent_packets}"
    print("Assertion passed: display name 'KEXP' (no .pls) is sent")
    assertions_passed.append("library_browser_radio_display_name")

    # Reset lists
    mpd_lsinfo_calls.clear()
    sent_packets.clear()

    print("\n--- Testing library_browser: SET_RADIO_BROWSE(False) ---")
    lb.set_radio_browse(False)
    lb.handle_browse_request([0xFFFF]) # LIB_BROWSE_ROOT

    # Assert mpd_lsinfo got ""
    assert "" in mpd_lsinfo_calls, f"Expected mpd_lsinfo_calls to contain '', got {mpd_lsinfo_calls}"
    print("Assertion passed: mpd_lsinfo got ''")
    assertions_passed.append("library_browser_root_browse_path")

    # Assert normal entry is Track.flac
    assert len(sent_packets) > 0, "No packets sent"
    found_track = False
    for pkt in sent_packets:
        msg_type, seq, cmd_id, payload = pkt
        if msg_type == fake_protocol.MSG_LIBRARY_ENTRY:
            entry_type = payload[0]
            index, total = struct.unpack("<HH", payload[1:5])
            name = payload[5:].decode("utf-8")
            print(f"Sent Entry: type={entry_type}, index={index}, total={total}, name={name!r}")
            if name == "Track.flac":
                found_track = True
    assert found_track, f"Expected display name 'Track.flac', but it was not found in sent packets"
    print("Assertion passed: normal entry displayed as 'Track.flac'")
    assertions_passed.append("library_browser_normal_display_name")

except AssertionError as ae:
    print(f"Assertion failed during library_browser phase: {ae}")
    sys.exit(1)


# --- Phase 2: Test uart5_listener.py separately ---
print("\n--- Testing uart5_listener ---")

# Let's clean up sys.modules to import uart5_listener.py cleanly with mocked environment
for mod in ["library_browser", "protocol", "uart_writer_client", "mpd_client", "uart5_listener", "command_ids", "panel_control", "playback_commands", "serial", "RPi", "RPi.GPIO"]:
    if mod in sys.modules:
        del sys.modules[mod]

# Mock modules for Phase 2
mock_set_radio_browse_calls = []
def mock_set_radio_browse(enabled):
    print(f"Mock set_radio_browse called with: {enabled}")
    mock_set_radio_browse_calls.append(enabled)

fake_lb = ModuleType("library_browser")
fake_lb.set_radio_browse = mock_set_radio_browse
fake_lb.handle_browse_request = lambda params: None
fake_lb.handle_add_track = lambda params: None
fake_lb.handle_playlist_request = lambda params: None
fake_lb.handle_play_track = lambda params: None
sys.modules["library_browser"] = fake_lb

# Mock command_ids
fake_cmd = ModuleType("command_ids")
fake_cmd.CMD_SYS_RPI_SHUTDOWN = 0x0002
fake_cmd.CMD_NEXT_TRACK = 0x0100
fake_cmd.CMD_PREVIOUS_TRACK = 0x0101
fake_cmd.CMD_PLAY_PAUSE = 0x0102
fake_cmd.CMD_STOP_TRACK = 0x0103
fake_cmd.CMD_SKIP_FORWARD = 0x0104
fake_cmd.CMD_SKIP_BACK = 0x0105
fake_cmd.CMD_PREV_MENU_ITEM = 0x0106
fake_cmd.CMD_NEXT_MENU_ITEM = 0x0107
fake_cmd.CMD_ROTARY_ACTION = 0x0112
fake_cmd.CMD_TOGGLE_METER = 0x0115
fake_cmd.CMD_TOGGLE_COVER_VIEW = 0x0119
fake_cmd.CMD_TOGGLE_REPEAT = 0x011C
fake_cmd.CMD_TOGGLE_RANDOM = 0x011F
fake_cmd.CMD_SELECT_PANEL_PLAYBACK = 0x0122
fake_cmd.CMD_SELECT_PANEL_RADIO    = 0x0123
fake_cmd.CMD_SELECT_PANEL_PLAYLIST = 0x0124
fake_cmd.CMD_SELECT_PANEL_FOLDER   = 0x0125
fake_cmd.CMD_SELECT_PANEL_TAG      = 0x0126
fake_cmd.CMD_SELECT_PANEL_ALBUM    = 0x0127
fake_cmd.CMD_BROWSE_REQUEST = 0x0128
fake_cmd.CMD_ADD_TRACK = 0x0129
fake_cmd.CMD_PLAYLIST_REQUEST = 0x012A
fake_cmd.CMD_PLAY_TRACK = 0x012B
sys.modules["command_ids"] = fake_cmd

# Mock panel_control
panel_select_panel_calls = []
def mock_make_select_panel_handler(panel_index):
    def dummy_handler(params):
        panel_select_panel_calls.append((panel_index, params))
    return dummy_handler

fake_panel = ModuleType("panel_control")
fake_panel.make_select_panel_handler = mock_make_select_panel_handler
fake_panel.handle_prev_panel = lambda params: None
fake_panel.handle_next_panel = lambda params: None
sys.modules["panel_control"] = fake_panel

# Mock playback_commands
fake_playback = ModuleType("playback_commands")
fake_playback.handle_rpi_shutdown = lambda params: None
fake_playback.handle_next_track = lambda params: None
fake_playback.handle_previous_track = lambda params: None
fake_playback.handle_play_pause = lambda params: None
fake_playback.handle_stop_track = lambda params: None
fake_playback.handle_skip_forward = lambda params: None
fake_playback.handle_skip_back = lambda params: None
fake_playback.handle_rotary_action = lambda params: None
fake_playback.toggle_meter_display = lambda params: None
fake_playback.toggle_cover_view = lambda params: None
fake_playback.toggle_repeat = lambda params: None
fake_playback.toggle_random = lambda params: None
fake_playback.set_meter_display = lambda enabled: None
sys.modules["playback_commands"] = fake_playback

# Mock protocol
fake_protocol2 = ModuleType("protocol")
fake_protocol2.compute_checksum = lambda data: 0
fake_protocol2.read_packet_with_resync = lambda ser: b""
sys.modules["protocol"] = fake_protocol2

# Mock serial
fake_serial = ModuleType("serial")
class DummySerial:
    def __init__(self, port, baud, timeout):
        pass
fake_serial.Serial = DummySerial
sys.modules["serial"] = fake_serial

# Mock GPIO
fake_gpio = ModuleType("RPi.GPIO")
fake_gpio.BCM = 11
fake_gpio.IN = 1
fake_gpio.PUD_DOWN = 21
fake_gpio.setmode = lambda mode: None
fake_gpio.setup = lambda pin, mode, pull_up_down=None: None
fake_gpio.wait_for_edge = lambda pin, edge, timeout=None: None
fake_gpio.cleanup = lambda: None
sys.modules["RPi"] = ModuleType("RPi")
sys.modules["RPi.GPIO"] = fake_gpio

# Finally, import uart5_listener
try:
    import uart5_listener as ul
except Exception as e:
    print(f"Failed to import uart5_listener: {e}")
    sys.exit(1)

# "Invoke its radio handler and then playback handler and assert set_radio_browse receives True then False."
try:
    radio_handler = ul.COMMAND_HANDLERS.get(fake_cmd.CMD_SELECT_PANEL_RADIO)
    playback_handler = ul.COMMAND_HANDLERS.get(fake_cmd.CMD_SELECT_PANEL_PLAYBACK)

    assert radio_handler is not None, "Radio handler (CMD_SELECT_PANEL_RADIO) not found in COMMAND_HANDLERS"
    assert playback_handler is not None, "Playback handler (CMD_SELECT_PANEL_PLAYBACK) not found in COMMAND_HANDLERS"

    print("Invoking radio handler...")
    radio_handler([0])
    
    print("Invoking playback handler...")
    playback_handler([0])

    print(f"set_radio_browse calls: {mock_set_radio_browse_calls}")
    assert mock_set_radio_browse_calls == [True, False], f"Expected mock_set_radio_browse_calls to be [True, False], got {mock_set_radio_browse_calls}"
    print("Assertion passed: set_radio_browse called with True then False")
    assertions_passed.append("uart5_listener_set_radio_browse")

except AssertionError as ae:
    print(f"Assertion failed during uart5_listener phase: {ae}")
    sys.exit(1)

print("\n=== ALL TESTS PASSED SUCCESSFULLY! ===")
print(f"Passed assertions count: {len(assertions_passed)}")
print("Assertions list:", assertions_passed)
