import serial
import time
import RPi.GPIO as GPIO
import os
import struct

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"   # UART5
BAUD_RATE = 115200
DRDY_PIN = 23

UART_START_BYTE = 0xAA
PACKET_SIZE = 18
PACKET_FORMAT = "<BBBBBH5HB"  # little-endian

# === STATE ===
last_brightness_value = 50  # initial brightness

# === GPIO SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

# === UART SETUP ===
ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)

# === FUNCTIONS ===

def compute_checksum_cpp_style(packet_bytes):
    """Sum of bytes 1..16 (skip start byte), modulo 256"""
    return sum(packet_bytes[1:17]) & 0xFF


def read_packet_with_resync():
    """
    Reads exactly one 18-byte packet from UART, resynchronizing on start byte (0xAA).
    Returns bytes object of length 18.
    """
    while True:
        b = ser.read(1)
        if not b:
            time.sleep(0.001)
            continue
        if b[0] == UART_START_BYTE:
            rest = ser.read(PACKET_SIZE - 1)
            while len(rest) < (PACKET_SIZE - 1):
                chunk = ser.read((PACKET_SIZE - 1) - len(rest))
                if chunk:
                    rest += chunk
                else:
                    time.sleep(0.001)
            return bytes([UART_START_BYTE]) + rest


def handle_command(command_id, params):
    global last_brightness_value

    print(f"Handling Command ID: {command_id:#06x}, Params: {params}", flush=True)

    if command_id == 0x0002:   
        os.system("moodeutl --shutdown")         # Shutdown

    elif command_id == 0x0100:          # Next track
        os.system("mpc next")

    elif command_id == 0x0101:          # Previous track
        os.system("mpc prev")

    elif command_id == 0x0102:          # Play/Pause
        os.system("mpc toggle")

    elif command_id == 0x0103:          # Stop
        os.system("mpc stop")

    elif command_id == 0x0104:          # Seek +10s
        os.system("mpc seek +10")

    elif command_id == 0x0105:          # Seek -10s
        os.system("mpc seek -10")
    
    elif command_id == 0x0115:
        if params[0] == 1:
            os.system("sudo moodeutl --setdisplay peppy")
        else:
            os.system("sudo moodeutl --setdisplay webui")          

    elif command_id == 0x0116:          
        if last_brightness_value >= 100:
            last_brightness_value = 10
        else:
            last_brightness_value += 10
        os.system(f"ddcutil setvcp 10 {last_brightness_value}")

    elif command_id == 0x010E:          # Display ON/OFF
        if params[0] == 1:
            os.system("ddcutil setvcp d6 1")  # ON
        else:
            os.system("ddcutil setvcp d6 5")  # OFF

    else:
        print("⚠️ Unknown command", flush=True)


def read_and_process_packet():
    """Read a full UART packet and process it."""
    data = read_packet_with_resync()
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

    # Validate checksum
    calc_checksum = compute_checksum_cpp_style(data)
    if checksum != calc_checksum:
        print("⚠️ Checksum mismatch", flush=True)
        print("Raw packet:", [hex(b) for b in data])
        return

    # Print packet info
    print("📦 Packet received:", flush=True)
    print(f"  Version: {version}, Source App: {src_app}, Msg Type: {msg_type}, Seq: {sequence}")
    print(f"  Command ID: {cmd_id:#06x}, Params: {params}, Checksum: {checksum}")

    # Execute command
    handle_command(cmd_id, params)


def wait_for_data_ready():
    """Wait for DRDY rising edge, with timeout to avoid deadlock"""
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
