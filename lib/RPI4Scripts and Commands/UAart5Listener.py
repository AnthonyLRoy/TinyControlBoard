import serial
import time
import RPi.GPIO as GPIO
import os
import struct

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"  # UART5
BAUD_RATE = 115200
DRDY_PIN = 23

# === GPIO SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

# === UART SETUP ===
ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)

# === PROTOCOL ===
# start_byte(1B) | version(1B) | src_app(1B) | msg_type(1B) |
# sequence(1B) | command_id(2B) | params[5] (2B each) | checksum(1B)
PACKET_SIZE = 18
PACKET_FORMAT = "<BBBBBH5HB"  # little-endian
UART_START_BYTE = 0xAA  # match C++ start byte

# === FUNCTIONS ===

def compute_checksum_cpp_style(packet_bytes):
    """
    Compute checksum like the C++ sender:
    sum of bytes 1..16 (skip start_byte), modulo 256
    """
    return sum(packet_bytes[1:17]) % 256

def read_full_packet():
    """Read exactly PACKET_SIZE bytes from UART, accumulating partial reads."""
    data = bytearray()
    while len(data) < PACKET_SIZE:
        chunk = ser.read(PACKET_SIZE - len(data))
        if chunk:
            data.extend(chunk)
        else:
            time.sleep(0.001)
    return bytes(data)

def handle_command(command_id, params):
    """Execute Moode commands based on command_id."""
    print(f"Handling Command ID: {command_id}, Params: {params}", flush=True)
    if command_id == 0x0102:
        os.system("mpc toggle")
    elif command_id == 0x0100:
        os.system("mpc next")
    elif command_id ==  0x0102:
        os.system("mpc toggle")
    elif command_id == 0x0103:
        os.system("mpc stop")
    elif command_id == 0x0104:
        os.system("mpc +10")
    elif command_id == 0x0105:
        os.system("mpc -10")
  
    else:
        print("Unknown command", flush=True)

def read_and_process_packet():
    """Read a full UART packet and process it."""
    data = read_full_packet()
    try:
        fields = struct.unpack(PACKET_FORMAT, data)
    except struct.error as e:
        print(f"⚠️ Struct unpack error: {e}, data length={len(data)}", flush=True)
        return

    # Unpack fields
    start_byte, version, src_app, msg_type, sequence, cmd_id, *params, checksum = fields

    # Validate start byte
    if start_byte != UART_START_BYTE:
        print(f"⚠️ Invalid start byte: {start_byte:#02x}", flush=True)
        return

    # Validate checksum using C++ style
    calc_checksum = compute_checksum_cpp_style(data)
    if checksum != calc_checksum:
        print("Raw packet:", [hex(b) for b in data])
        print(f"⚠️ Checksum mismatch: received={checksum}, calculated={calc_checksum}", flush=True)
        return

    # Print full packet contents
    print("📦 Packet received:", flush=True)
    print(f"  Start Byte: {start_byte:#02x}", flush=True)
    print(f"  Version: {version}", flush=True)
    print(f"  Source App: {src_app}", flush=True)
    print(f"  Msg Type: {msg_type}", flush=True)
    print(f"  Sequence: {sequence}", flush=True)
    print(f"  Command ID: {cmd_id}", flush=True)
    print(f"  Params: {params}", flush=True)
    print(f"  Checksum: {checksum}", flush=True)

    # Handle the command
    handle_command(cmd_id, params)

def wait_for_data_ready():
    """Wait for DRDY pin to rise."""
    GPIO.wait_for_edge(DRDY_PIN, GPIO.RISING)

# === MAIN LOOP ===
print("🎧 UART5 listener started — waiting for data...", flush=True)

try:
    while True:
        wait_for_data_ready()
        read_and_process_packet()
        time.sleep(0.001)
except KeyboardInterrupt:
    print("Exiting...", flush=True)
finally:
    ser.close()
    GPIO.cleanup()
