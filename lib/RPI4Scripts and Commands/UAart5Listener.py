import serial
import time
import RPi.GPIO as GPIO
import os
import struct
import math

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"   # UART5
BAUD_RATE = 115200
DRDY_PIN = 23



UART_START_BYTE = 0xAA
PACKET_SIZE = 18
PACKET_FORMAT = "<BBBBBH5HB"  # little-endian



# === GPIO SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)



# calculate the checksum to ensure data not currupted 

def compute_checksum_cpp_style(packet_bytes):
    return sum(packet_bytes[1:17]) & 0xFF

def read_packet_with_resync():
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
    print(f"Handling Command ID: {command_id:#06x}, Params: {params}", flush=True)

    if command_id == 0x0002:
        os.system("sudo moodeutl --shutdown")
    elif command_id == 0x0100:
        os.system("mpc next")
    elif command_id == 0x0101:
        os.system("mpc prev")
    elif command_id == 0x0102:
        os.system("mpc toggle")
    elif command_id == 0x0103:
        os.system("mpc stop")
    elif command_id == 0x0104:
        os.system("mpc seek +10")
    elif command_id == 0x0105:
        os.system("mpc seek -10")
    elif command_id == 0x0115:
        if params[0] == 1:
            os.system("sudo moodeutl --setdisplay peppy")
        else:
            os.system("sudo moodeutl --setdisplay webui")
    elif command_id == 0x0112:
        if params[0] == 1:
            os.system("mpc next")
        else:
            os.system("mpc prev")
    elif command_id == 0x0119:
        if params[0] == 1:
            os.system("/var/www/util/coverview.php -on")
        else:
            os.system("/var/www/util/coverview.php -off")
    else:
        print("⚠️ Unknown command", flush=True)

def read_and_process_packet():
    data = read_packet_with_resync()
    try:
        fields = struct.unpack(PACKET_FORMAT, data)
    except struct.error as e:
        print(f"⚠️ Struct unpack error: {e}", flush=True)
        return

    start_byte, version, src_app, msg_type, sequence, cmd_id, *params, checksum = fields

    if start_byte != UART_START_BYTE:
        return

    if checksum != compute_checksum_cpp_style(data):
        print("⚠️ Checksum mismatch", flush=True)
        return

    handle_command(cmd_id, params)

def wait_for_data_ready():
    GPIO.wait_for_edge(DRDY_PIN, GPIO.RISING)

# === UART SETUP ===
ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)

# === MAIN LOOP ===
print("🎧 UART5 listener started", flush=True)

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
