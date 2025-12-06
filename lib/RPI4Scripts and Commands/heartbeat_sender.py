import serial
import time
import struct
import RPi.GPIO as GPIO

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"
BAUD_RATE = 115200

DRDY_PIN = 24  # ESP32 data ready/busy line
BLIP_TIME = 0.002  # 2 ms "data ready" pulse

# === PROTOCOL ===
PACKET_FORMAT = "<BBBBBH5HB"
UART_START_BYTE = 0xAA
VERSION = 0x01
SRC_APP = 0x02
MSG_TYPE = 0x01
CMD_ID_HEARTBEAT = 0x9999
PARAMS = [0, 0, 0, 0, 0]

# === SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)

def compute_checksum(packet_bytes):
    return sum(packet_bytes[1:17]) % 256

def build_heartbeat(seq):
    partial = struct.pack(
        PACKET_FORMAT,
        UART_START_BYTE,
        VERSION,
        SRC_APP,
        MSG_TYPE,
        seq,
        CMD_ID_HEARTBEAT,
        *PARAMS,
        0  # placeholder
    )
    checksum = compute_checksum(partial)
    return struct.pack(
        PACKET_FORMAT,
        UART_START_BYTE,
        VERSION,
        SRC_APP,
        MSG_TYPE,
        seq,
        CMD_ID_HEARTBEAT,
        *PARAMS,
        checksum
    )

def wait_until_low():
    """Block until ESP32 is ready (DRDY = 0)."""
    while GPIO.input(DRDY_PIN) == GPIO.HIGH:
        time.sleep(0.001)

def blip():
    """Notify ESP32 data is ready by brief change on DRDY pin."""
    GPIO.setup(DRDY_PIN, GPIO.OUT)
    GPIO.output(DRDY_PIN, GPIO.HIGH)
    time.sleep(BLIP_TIME)
    GPIO.output(DRDY_PIN, GPIO.LOW)
    GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

print("🔥 Heartbeat sender running...")

seq = 0

try:
    while True:
        wait_until_low()          # Wait until ESP32 is idle
        blip()                    # Notify ESP32 a packet is coming

        pkt = build_heartbeat(seq)
        ser.write(pkt)
        print(f"Heartbeat sent (seq={seq})", flush=True)

        seq = (seq + 1) & 0xFF
        time.sleep(1.0)

except KeyboardInterrupt:
    print("Exiting heartbeat sender...")

finally:
    ser.close()
    GPIO.cleanup()
