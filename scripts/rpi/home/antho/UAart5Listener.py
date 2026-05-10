import serial
import time
import RPi.GPIO as GPIO
import struct
import subprocess

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
CMD_ROTARY_ACTION = 0x0112
CMD_TOGGLE_METER = 0x0115
CMD_TOGGLE_COVER_VIEW = 0x0119
CMD_TOGGLE_REPEAT = 0x011C

# === PARAMETER VALUES ===
PARAM_DISABLED = 0
PARAM_ENABLED = 1
ROTARY_ACTION_PREVIOUS = 0
ROTARY_ACTION_NEXT = 1
# ================================
# UART packet structure:
# [0] Start Byte (0xAA) 
UART_START_BYTE = 0xAA
PACKET_SIZE = 18
PACKET_FORMAT = "<BBBBBH5HB"  # little-endian



# calculate the checksum to ensure data not currupted 

def compute_checksum_cpp_style(packet_bytes):
    return sum(packet_bytes[1:17]) & 0xFF

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

COMMAND_HANDLERS = {
    CMD_SYS_RPI_SHUTDOWN: lambda params: run_command(["sudo", "moodeutl", "--shutdown"]),
    CMD_NEXT_TRACK: lambda params: run_command(["mpc", "next"]),
    CMD_PREVIOUS_TRACK: lambda params: run_command(["mpc", "prev"]),
    CMD_PLAY_PAUSE: lambda params: run_command(["mpc", "toggle"]),
    CMD_STOP_TRACK: lambda params: run_command(["mpc", "stop"]),
    CMD_SKIP_FORWARD: lambda params: run_command(["mpc", "seek", "+10"]),
    CMD_SKIP_BACK: lambda params: run_command(["mpc", "seek", "-10"]),
    CMD_ROTARY_ACTION: handle_rotary_action,
    CMD_TOGGLE_METER: toggle_meter_display,
    CMD_TOGGLE_COVER_VIEW: toggle_cover_view,
    CMD_TOGGLE_REPEAT: toggle_repeat,
}

def setup_gpio():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

def read_packet_with_resync(ser):
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

    handler = COMMAND_HANDLERS.get(command_id)
    if handler is None:
        print("⚠️ Unknown command", flush=True)
        return

    handler(params)

def read_and_process_packet(ser):
    data = read_packet_with_resync(ser)
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

def main():
    ser = None

    try:
        setup_gpio()
        ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)
        set_meter_display(DEFAULT_METER_ENABLED)
        print("🎧 UART5 listener started", flush=True)

        while True:
            wait_for_data_ready()
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
