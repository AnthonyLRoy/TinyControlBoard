import serial
import time
import struct
import subprocess
import re
import os
import socket
import threading
import RPi.GPIO as GPIO

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"
BAUD_RATE = 115200
HEARTBEAT_INTERVAL_S = 10.0
NOW_PLAYING_INTERVAL_S = 5.0
TRACK_PROGRESS_INTERVAL_S = 2.0
TICK_S = 1.0  # main-loop granularity; must be <= the smallest interval above

DRDY_PIN = 24  # ESP32 data ready/busy line
BLIP_TIME = 0.002  # 2 ms "data ready" pulse
# lgpio (the RPi.GPIO backend on current Raspberry Pi OS) only allows ONE process to
# claim a given GPIO line at a time — so this process is now the sole owner of DRDY_PIN
# and the serial port. uart5_listener.py forwards its outbound packets (library entries)
# to UART_WRITER_SOCK_PATH instead of touching GPIO/serial itself.
UART_WRITER_SOCK_PATH = "/tmp/tinycontrolboard_uart_writer.sock"

# === PROTOCOL ===
# Variable-length packet: [start, version, src, type, seq, cmd_lo, cmd_hi, payload_len, ...payload, checksum]
HEADER_FORMAT = "<BBBBBHB"  # 8 bytes (includes payload_len)
UART_START_BYTE = 0xAA
VERSION = 0x01
SRC_APP = 0x02
MSG_COMMAND        = 0x01
MSG_NOW_PLAYING    = 0x05
MSG_TRACK_PROGRESS = 0x06
CMD_ID_HEARTBEAT = 0x0003

# === GPIO SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)
# ok this is not the official way of writing and reading checksums but it work 
def compute_checksum(data):
    """Sum bytes [1..7+payload_len] (everything between start byte and checksum)."""
    payload_len = data[7]
    return sum(data[1:8 + payload_len]) % 256

def build_packet(msg_type, seq, cmd_id, payload=b''):
    header = struct.pack(HEADER_FORMAT,
                         UART_START_BYTE, VERSION, SRC_APP,
                         msg_type, seq, cmd_id, len(payload))
    body = header + payload
    return body + bytes([compute_checksum(body)])

def wait_until_low():
    """Block until ESP32 is ready (DRDY = 0)."""
    while GPIO.input(DRDY_PIN) == GPIO.HIGH:
        time.sleep(0.001)

def blip():
    """Notify ESP32 data is ready by brief pulse on DRDY pin."""
    GPIO.setup(DRDY_PIN, GPIO.OUT)
    GPIO.output(DRDY_PIN, GPIO.HIGH)
    time.sleep(BLIP_TIME)
    GPIO.output(DRDY_PIN, GPIO.LOW)
    GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

_send_lock = threading.Lock()  # guards GPIO24 + ser between the main loop and the writer socket thread

def send_packet(pkt):
    with _send_lock:
        wait_until_low()
        ser.write(pkt)
        ser.flush()
        blip()

def _recv_exact(conn, n):
    buf = b""
    while len(buf) < n:
        chunk = conn.recv(n - len(buf))
        if not chunk:
            return None
        buf += chunk
    return buf

def _run_uart_writer_server():
    """Accepts already-framed packets from uart5_listener.py over a local Unix socket
    and sends them via this process's exclusively-owned GPIO24 + serial port."""
    try:
        os.remove(UART_WRITER_SOCK_PATH)
    except FileNotFoundError:
        pass
    srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv.bind(UART_WRITER_SOCK_PATH)
    os.chmod(UART_WRITER_SOCK_PATH, 0o666)
    srv.listen(5)
    while True:
        conn, _ = srv.accept()
        try:
            length_bytes = _recv_exact(conn, 4)
            if length_bytes is None:
                continue
            pkt = _recv_exact(conn, int.from_bytes(length_bytes, "big"))
            if pkt is not None:
                send_packet(pkt)
        except Exception as e:
            print(f"⚠️ uart-writer server error: {e}", flush=True)
        finally:
            conn.close()

threading.Thread(target=_run_uart_writer_server, daemon=True).start()

_TIME_RANGE_RE = re.compile(r"(\d+(?::\d+){1,2})/(\d+(?::\d+){1,2})")

def _parse_time_token(token):
    """Parses 'H:MM:SS' or 'M:SS' into total seconds."""
    seconds = 0
    for part in token.split(":"):
        seconds = seconds * 60 + int(part)
    return seconds

def get_mpc_status():
    """Returns the current MPD label, position, duration, and playing state."""
    try:
        result = subprocess.run(["mpc", "status"], capture_output=True, text=True, timeout=2)
        lines = result.stdout.splitlines()
        state_line = next((line for line in lines if "[playing]" in line or "[paused]" in line), None)
        if state_line is None:
            return "", 0, 0, False
        is_playing = "[playing]" in state_line
        track = ""
        if is_playing:
            stream_name = subprocess.run(
                ["mpc", "current", "--format", "%name%"],
                capture_output=True,
                text=True,
                timeout=2,
            ).stdout.strip()
            track = stream_name or next((line.strip() for line in lines if line != state_line), "")

        elapsed, duration = 0, 0
        match = _TIME_RANGE_RE.search(state_line)
        if match and ("[playing]" in state_line or "[paused]" in state_line):
            elapsed = _parse_time_token(match.group(1))
            duration = _parse_time_token(match.group(2))

        return track, elapsed, duration, is_playing
    except Exception:
        return "", 0, 0, False

print("Heartbeat sender running...", flush=True)

seq = 0
last_now_playing_time = 0.0
last_now_playing_text = None
last_track_progress_time = 0.0
last_heartbeat_time = 0.0

try:
    while True:
        now = time.monotonic()

        if now - last_heartbeat_time >= HEARTBEAT_INTERVAL_S:
            last_heartbeat_time = now
            send_packet(build_packet(MSG_COMMAND, seq, CMD_ID_HEARTBEAT))
            print(f"Heartbeat sent (seq={seq})", flush=True)
            seq = (seq + 1) & 0xFF

        need_now_playing = now - last_now_playing_time >= NOW_PLAYING_INTERVAL_S
        need_progress = now - last_track_progress_time >= TRACK_PROGRESS_INTERVAL_S

        if need_now_playing or need_progress:
            track, elapsed, duration, is_playing = get_mpc_status()

            if need_now_playing:
                last_now_playing_time = now
                if track != last_now_playing_text:
                    last_now_playing_text = track
                    payload = track.encode("utf-8")[:60]
                    send_packet(build_packet(MSG_NOW_PLAYING, seq, 0, payload))
                    print(f"Now playing sent: {track!r}", flush=True)
                    seq = (seq + 1) & 0xFF

            if need_progress:
                last_track_progress_time = now
                payload = struct.pack("<HHB", elapsed, duration, int(is_playing))
                send_packet(build_packet(MSG_TRACK_PROGRESS, seq, 0, payload))
                print(f"Track progress sent: {elapsed}s/{duration}s", flush=True)
                seq = (seq + 1) & 0xFF

        time.sleep(TICK_S)

except KeyboardInterrupt:
    print("Exiting heartbeat sender...")

finally:
    ser.close()
    GPIO.cleanup()
