import base64
import json
import os
import socket
import struct

import requests

PANELS = [
    ('#playbar-switch',    'Playback'),
    ('.radio-view-btn',    'Radio'),
    ('.playlist-view-btn', 'Playlist'),
    ('.folder-view-btn',   'Folder'),
    ('.tag-view-btn',      'Tag'),
    ('.album-view-btn',    'Album'),
]


class _PanelState:
    def __init__(self):
        self.panel_idx = 0
        self.cdp_ws_url = None

_state = _PanelState()


def _get_cdp_ws_url():
    """Returns (and caches) the Chrome DevTools debugger socket URL for the kiosk browser's first tab."""
    if _state.cdp_ws_url is None:
        targets = requests.get('http://localhost:9222/json', timeout=1).json()
        _state.cdp_ws_url = targets[0]['webSocketDebuggerUrl']
    return _state.cdp_ws_url


def _cdp_ws_handshake(sock, host, port, path):
    """Performs the HTTP Upgrade handshake required before a socket can carry WebSocket frames."""
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


def _build_ws_text_frame(payload):
    """Wraps payload bytes in a single masked WebSocket text frame (RFC 6455 requires client->server masking)."""
    mask = os.urandom(4)
    n = len(payload)
    frame = bytearray([0x81])  # FIN + text-frame opcode
    if n < 126:
        frame.append(0x80 | n)
    elif n < 65536:
        frame += bytearray([0x80 | 126]) + struct.pack('>H', n)
    else:
        frame += bytearray([0x80 | 127]) + struct.pack('>Q', n)
    frame += mask
    frame += bytearray(b ^ mask[i % 4] for i, b in enumerate(payload))
    return bytes(frame)


def _click_panel(css_selector):
    try:
        ws_url = _get_cdp_ws_url()

        # Parse ws://host:port/path
        url = ws_url[5:]  # strip 'ws://'
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
        _cdp_ws_handshake(sock, host, port, path)
        sock.sendall(_build_ws_text_frame(payload))
        sock.close()
    except Exception as e:
        _state.cdp_ws_url = None
        print(f"Panel switch failed: {e}", flush=True)


def _go_to_panel(idx):
    _state.panel_idx = idx
    selector, name = PANELS[idx]
    print(f"Panel → {name}", flush=True)
    _click_panel(selector)


def make_select_panel_handler(idx):
    def _handler(params):
        _go_to_panel(idx)
    return _handler


def handle_next_panel(params):
    _go_to_panel((_state.panel_idx + 1) % len(PANELS))


def handle_prev_panel(params):
    _go_to_panel((_state.panel_idx - 1) % len(PANELS))
