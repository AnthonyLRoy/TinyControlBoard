import serial
import time
import RPi.GPIO as GPIO
import os
import struct
import pigpio
import math

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"   # UART5
BAUD_RATE = 115200
DRDY_PIN = 23

# === PWM CONFIG ===
PWM_PIN = 26          # GPIO26
PWM_FREQ = 2000       # 2 kHz backlight PWM

UART_START_BYTE = 0xAA
PACKET_SIZE = 18
PACKET_FORMAT = "<BBBBBH5HB"  # little-endian

# === STATE ===
last_brightness_value = 50  # initial brightness (percent)
wave_id = None              # pigpio waveform id

# === GPIO SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

# === pigpio SETUP ===
pi = pigpio.pi()
if not pi.connected:
    raise RuntimeError("pigpio daemon not running")

pi.set_mode(PWM_PIN, pigpio.OUTPUT)

# === WAVEFORM PWM FUNCTIONS ===
def create_pwm_wave(duty_percent):
    global wave_id

    duty_percent = max(0, min(100, duty_percent))
    gamma = 2.2
    duty_gamma = (duty_percent / 100) ** gamma
    period_us = int(1_000_000 / PWM_FREQ)
    on_time = int(period_us * duty_gamma)
    off_time = period_us - on_time

    # Clear previous waveform
    if wave_id is not None:
        pi.wave_tx_stop()
        pi.wave_delete(wave_id)
        wave_id = None

    # Build waveform pulses
    pulses = [
        pigpio.pulse(1 << PWM_PIN, 0, on_time),
        pigpio.pulse(0, 1 << PWM_PIN, off_time)
    ]
    pi.wave_add_generic(pulses)
    wave_id = pi.wave_create()
    if wave_id >= 0:
        pi.wave_send_repeat(wave_id)
    else:
        print("⚠️ Failed to create waveform", flush=True)

def set_brightness_pwm(percent):
    create_pwm_wave(percent)

# Set initial brightness
set_brightness_pwm(last_brightness_value)

# === FUNCTIONS ===

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
    global last_brightness_value

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

    # === Brightness cycle (PWM) ===
    elif command_id == 0x0116:
        if last_brightness_value >= 100:
            last_brightness_value = 10
        else:
            last_brightness_value += 10

        set_brightness_pwm(last_brightness_value)
        print(f"🔆 Brightness set to {last_brightness_value}%", flush=True)

    # === Display ON / OFF (backlight only) ===
    elif command_id == 0x010E:
        if params[0] == 1:
            set_brightness_pwm(last_brightness_value)
            print("🟢 Display ON", flush=True)
        else:
            set_brightness_pwm(0)
            print("🔴 Display OFF", flush=True)
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
print("🎧 UART5 listener started — PWM on GPIO26", flush=True)

try:
    while True:
        wait_for_data_ready()
        read_and_process_packet()
        time.sleep(0.001)

except KeyboardInterrupt:
    print("Exiting...", flush=True)

finally:
    if wave_id is not None:
        pi.wave_tx_stop()
        pi.wave_delete(wave_id)
    pi.stop()
    ser.close()
    GPIO.cleanup()
