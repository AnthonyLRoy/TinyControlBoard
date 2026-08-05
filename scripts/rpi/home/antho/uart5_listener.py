import serial
import time
import RPi.GPIO as GPIO
import struct
import subprocess
import json
import os
import socket
import base64
import requests

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"   # UART5
BAUD_RATE = 115200
DRDY_PIN = 23
DEFAULT_METER_ENABLED = False
# ==========  command IDs ==========
CMD_SYS_RPI_SHUTDOWN = 0x0002
CMD_NEXT_TRACK = 0x0100
CMD_PREVIOUS_TRACK = 0x0101
CMD_PLAY_PAUSE = 0x0102
CMD_STOP_TRACK = 0x0103
CMD_SKIP_FORWARD = 0x0104
CMD_SKIP_BACK = 0x0105
CMD_PREV_MENU_ITEM = 0x0106
CMD_NEXT_MENU_ITEM = 0x0107
CMD_ROTARY_ACTION = 0x0112
CMD_TOGGLE_METER = 0x0115
CMD_TOGGLE_COVER_VIEW = 0x0119
CMD_TOGGLE_REPEAT = 0x011C
CMD_TOGGLE_RANDOM = 0x011F
CMD_SELECT_PANEL_PLAYBACK = 0x0122
CMD_SELECT_PANEL_RADIO    = 0x0123
CMD_SELECT_PANEL_PLAYLIST = 0x0124
CMD_SELECT_PANEL_FOLDER   = 0x0125
CMD_SELECT_PANEL_TAG      = 0x0126
CMD_SELECT_PANEL_ALBUM    = 0x0127

# === PANEL NAVIGATION ===
PANELS = [
    ('#playbar-switch',    'Playback'),
    ('.radio-view-btn',    'Radio'),
    ('.playlist-view-btn', 'Playlist'),
    ('.folder-view-btn',   'Folder'),
    ('.tag-view-btn',      'Tag'),
    ('.album-view-btn',    'Album'),
]
_panel_idx = 0
_cdp_ws_url = None

# === PARAMETER VALUES ===
PARAM_DISABLED = 0
PARAM_ENABLED = 1
ROTARY_ACTION_PREVIOUS = 0
ROTARY_ACTION_NEXT = 1
# ================================
# Variable-length packet structure:
# [0]      start byte (0xAA)
# [1]      version
# [2]      src_app
# [3]      msg_type
# [4]      sequence
# [5-6]    command_id (LE uint16)
# [7]      payload_len (N)
# [8..8+N-1]  payload
# [8+N]    checksum = sum(bytes[1..7+N]) % 256
UART_START_BYTE = 0xAA
HEADER_SIZE = 8  # bytes 0-7
HEADER_FORMAT = "<BBBBBHB"  # start, version, src, type, seq, cmd_id, payload_len

def _click_panel(css_selector):
    global _cdp_ws_url
    try:
        if _cdp_ws_url is None:
            targets = requests.get('http://localhost:9222/json', timeout=1).json()
            _cdp_ws_url = targets[0]['webSocketDebuggerUrl']

        # Parse ws://host:port/path
        url = _cdp_ws_url[5:]  # strip 'ws://'
        slash_idx = url.index('/')
        host_port = url[:slash_idx]
        path = url[slash_idx:]
        host, port_str = host_port.split(':')
        port = int(port_str)

        expression = f"document.querySelector('{css_selector}').click()"
        payload = json.dumps({
            'id': 1,
            'method': 'Runtime.evaluate',
            'params': {'expression': expression}
        }).encode()

        # Raw WebSocket upgrade — no Origin header sent
        sock = socket.create_connection((host, port), timeout=2)
        key = base64.b64encode(os.urandom(16)).decode()
        handshake = (
            f"GET {path} HTTP/1.1\r\n"
            f"Host: {host}:{port}\r\n"
            f"Upgrade: websocket\r\n"
            f"Connection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\n"
            f"Sec-WebSocket-Version: 13\r\n"
            f"\r\n"
        )
        sock.sendall(handshake.encode())

        buf = b''
        while b'\r\n\r\n' not in buf:
            buf += sock.recv(1024)
        if b'101' not in buf:
            raise Exception(f"WS handshake failed: {buf[:100]}")

        # Build masked WebSocket text frame
        mask = os.urandom(4)
        n = len(payload)
        frame = bytearray([0x81])
        if n < 126:
            frame.append(0x80 | n)
        elif n < 65536:
            frame += bytearray([0x80 | 126]) + struct.pack('>H', n)
        else:
            frame += bytearray([0x80 | 127]) + struct.pack('>Q', n)
        frame += mask
        frame += bytearray(b ^ mask[i % 4] for i, b in enumerate(payload))

        sock.sendall(bytes(frame))
        sock.close()
    except Exception as e:
        _cdp_ws_url = None
        print(f"Panel switch failed: {e}", flush=True)

def _make_select_panel_handler(idx):
    def _handler(params):
        global _panel_idx
        _panel_idx = idx
        selector, name = PANELS[idx]
        print(f"Panel → {name}", flush=True)
        _click_panel(selector)
    return _handler

def handle_next_panel(params):
    global _panel_idx
    _panel_idx = (_panel_idx + 1) % len(PANELS)
    selector, name = PANELS[_panel_idx]
    print(f"Panel → {name}", flush=True)
    _click_panel(selector)

def handle_prev_panel(params):
    global _panel_idx
    _panel_idx = (_panel_idx - 1) % len(PANELS)
    selector, name = PANELS[_panel_idx]
    print(f"Panel → {name}", flush=True)
    _click_panel(selector)

def compute_checksum_cpp_style(data):
    payload_len = data[7]
    return sum(data[1:8 + payload_len]) % 256

def run_command(args):
    result = subprocess.run(args, check=False)
    if result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}: {args}", flush=True)

def set_meter_display(enabled):
    if enabled:
        run_command(["sudo", "moodeutl", "--setdisplay", "peppy"])
    else:
        run_command(["sudo", "moodeutl", "--setdisplay", "webui"])

def toggle_meter_display(params):
    set_meter_display(params[0] == PARAM_ENABLED)

def handle_rotary_action(params):
    if params[0] == ROTARY_ACTION_NEXT:
        run_command(["mpc", "next"])
    else:
        run_command(["mpc", "prev"])

def toggle_cover_view(params):
    if params[0] == PARAM_ENABLED:
        run_command(["/var/www/util/coverview.php", "-on"])
    else:
        run_command(["/var/www/util/coverview.php", "-off"])

def toggle_repeat(params):
    if params[0] == PARAM_ENABLED:
        run_command(["mpc", "repeat", "on"])
    else:
        run_command(["mpc", "repeat", "off"])

def toggle_random(params):
    if params[0] == PARAM_ENABLED:
        run_command(["mpc", "random", "on"])
    else:
        run_command(["mpc", "random", "off"])

COMMAND_HANDLERS = {
    CMD_SYS_RPI_SHUTDOWN: lambda params: run_command(["sudo", "moodeutl", "--shutdown"]),
    CMD_NEXT_TRACK: lambda params: run_command(["mpc", "next"]),
    CMD_PREVIOUS_TRACK: lambda params: run_command(["mpc", "prev"]),
    CMD_PLAY_PAUSE: lambda params: run_command(["mpc", "toggle"]),
    CMD_STOP_TRACK: lambda params: run_command(["mpc", "stop"]),
    CMD_SKIP_FORWARD: lambda params: run_command(["mpc", "seek", "+10"]),
    CMD_SKIP_BACK: lambda params: run_command(["mpc", "seek", "-10"]),
    CMD_PREV_MENU_ITEM: handle_prev_panel,
    CMD_NEXT_MENU_ITEM: handle_next_panel,
    CMD_ROTARY_ACTION: handle_rotary_action,
    CMD_TOGGLE_METER: toggle_meter_display,
    CMD_TOGGLE_COVER_VIEW: toggle_cover_view,
    CMD_TOGGLE_REPEAT: toggle_repeat,
    CMD_TOGGLE_RANDOM: toggle_random,
    CMD_SELECT_PANEL_PLAYBACK: _make_select_panel_handler(0),
    CMD_SELECT_PANEL_RADIO:    _make_select_panel_handler(1),
    CMD_SELECT_PANEL_PLAYLIST: _make_select_panel_handler(2),
    CMD_SELECT_PANEL_FOLDER:   _make_select_panel_handler(3),
    CMD_SELECT_PANEL_TAG:      _make_select_panel_handler(4),
    CMD_SELECT_PANEL_ALBUM:    _make_select_panel_handler(5),
}

def setup_gpio():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

def read_packet_with_resync(ser):
    # Sync to start byte
    while True:
        b = ser.read(1)
        if not b:
            time.sleep(0.001)
            continue
        if b[0] == UART_START_BYTE:
            break

    # Read the rest of the header (bytes 1-7)
    header_rest = b''
    while len(header_rest) < HEADER_SIZE - 1:
        chunk = ser.read((HEADER_SIZE - 1) - len(header_rest))
        if chunk:
            header_rest += chunk
        else:
            time.sleep(0.001)

    header = bytes([UART_START_BYTE]) + header_rest
    payload_len = header[7]

    # Read payload + checksum
    remaining = payload_len + 1
    rest = b''
    while len(rest) < remaining:
        chunk = ser.read(remaining - len(rest))
        if chunk:
            rest += chunk
        else:
            time.sleep(0.001)

    return header + rest

def handle_command(command_id, params):
    print(f"Handling Command ID: {command_id:#06x}, Params: {params}", flush=True)

    handler = COMMAND_HANDLERS.get(command_id)
    if handler is None:
        print("⚠️ Unknown command", flush=True)
        return

    handler(params)

def read_and_process_packet(ser):
    data = read_packet_with_resync(ser)

    src_app = data[2]
    if src_app != 0x01:  # APP_ESP32; drop looped-back RPi transmissions
        print(f"⚠️ Dropping packet from src_app=0x{src_app:02X} (expected ESP32=0x01)", flush=True)
        return

    payload_len = data[7]
    checksum = data[8 + payload_len]

    if checksum != compute_checksum_cpp_style(data):
        print("⚠️ Checksum mismatch", flush=True)
        return

    cmd_id  = data[5] | (data[6] << 8)
    payload = data[8:8 + payload_len]

    # Decode params as 5×LE uint16 for command packets with ≥10 payload bytes
    params = [0, 0, 0, 0, 0]
    for i in range(min(5, payload_len // 2)):
        params[i] = payload[i * 2] | (payload[i * 2 + 1] << 8)

    handle_command(cmd_id, params)

def wait_for_data_ready():
    result = GPIO.wait_for_edge(DRDY_PIN, GPIO.RISING, timeout=10000)
    if result is None:
        print(f"⚠️ No DRDY pulse on GPIO {DRDY_PIN} in 10 s — ESP32 not sending", flush=True)

def main():
    ser = None

    try:
        setup_gpio()
        ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)
        set_meter_display(DEFAULT_METER_ENABLED)
        print("🎧 UART5 listener started", flush=True)

        while True:
            read_and_process_packet(ser)
            time.sleep(0.001)
    except KeyboardInterrupt:
        print("Exiting...", flush=True)
    finally:
        if ser is not None and ser.is_open:
            ser.close()
        GPIO.cleanup()

if __name__ == "__main__":
    main()
