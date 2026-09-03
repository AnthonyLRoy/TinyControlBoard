import struct
import time

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
PROTOCOL_VERSION = 0x01
SRC_APP_PI = 0x02
MSG_LIBRARY_ENTRY = 0x07
# ESP32->RPi only; payload = playlist name (UTF-8, no terminator). Only ever
# received (built by ESP32's BLE playlist-cmd characteristic), never sent by the RPi.
MSG_PLAYLIST_CMD = 0x08


def compute_checksum(data):
    payload_len = data[7]
    return sum(data[1:8 + payload_len]) % 256


def build_packet(msg_type, seq, cmd_id, payload=b''):
    header = struct.pack(HEADER_FORMAT,
                         UART_START_BYTE, PROTOCOL_VERSION, SRC_APP_PI,
                         msg_type, seq, cmd_id, len(payload))
    body = header + payload
    return body + bytes([compute_checksum(body)])


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
