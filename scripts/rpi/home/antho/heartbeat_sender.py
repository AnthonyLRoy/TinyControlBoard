import serial
import time
import struct
import subprocess
import RPi.GPIO as GPIO

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"
BAUD_RATE = 115200
HEARTBEAT_INTERVAL_S = 10.0
NOW_PLAYING_INTERVAL_S = 5.0

DRDY_PIN = 24  # ESP32 data ready/busy line
BLIP_TIME = 0.002  # 2 ms "data ready" pulse

# === PROTOCOL ===
# Variable-length packet: [start, version, src, type, seq, cmd_lo, cmd_hi, payload_len, ...payload, checksum]
HEADER_FORMAT = "<BBBBBHB"  # 8 bytes (includes payload_len)
UART_START_BYTE = 0xAA
VERSION = 0x01
SRC_APP = 0x02
MSG_COMMAND     = 0x01
MSG_NOW_PLAYING = 0x05
CMD_ID_HEARTBEAT = 0x0003

# === GPIO SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)

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

def send_packet(pkt):
    wait_until_low()
    ser.write(pkt)
    ser.flush()
    blip()

def get_current_track():
    """Returns the track name only when actively playing, empty string otherwise."""
    try:
        result = subprocess.run(["mpc", "status"], capture_output=True, text=True, timeout=2)
        lines = result.stdout.splitlines()
        # mpc status: line 0 = track name (if something is cued), line 1 = [playing]/[paused] + position
        if len(lines) >= 2 and "[playing]" in lines[1]:
            return lines[0].strip()
        return ""
    except Exception:
        return ""

print("Heartbeat sender running...", flush=True)

seq = 0
last_now_playing_time = 0.0
last_now_playing_text = None

try:
    while True:
        send_packet(build_packet(MSG_COMMAND, seq, CMD_ID_HEARTBEAT))
        print(f"Heartbeat sent (seq={seq})", flush=True)
        seq = (seq + 1) & 0xFF

        now = time.time()
        if now - last_now_playing_time >= NOW_PLAYING_INTERVAL_S:
            last_now_playing_time = now
            track = get_current_track()
            if track != last_now_playing_text:
                last_now_playing_text = track
                payload = track.encode("utf-8")[:60]
                send_packet(build_packet(MSG_NOW_PLAYING, seq, 0, payload))
                print(f"Now playing sent: {track!r}", flush=True)
                seq = (seq + 1) & 0xFF

        time.sleep(HEARTBEAT_INTERVAL_S)

except KeyboardInterrupt:
    print("Exiting heartbeat sender...")

finally:
    ser.close()
    GPIO.cleanup()
