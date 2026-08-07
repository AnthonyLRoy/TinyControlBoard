import socket

# GPIO24 (the ESP32 "data ready" line) + the serial write path are exclusively owned by
# heartbeat_sender.py (lgpio only allows one process to claim a GPIO line at a time).
# Outbound packets are forwarded to it over this Unix socket instead.
UART_WRITER_SOCK_PATH = "/tmp/tinycontrolboard_uart_writer.sock"


def send_packet_locked(pkt):
    """Forwards an already-framed packet to heartbeat_sender.py's UART-writer service,
    which exclusively owns GPIO24 + the serial port."""
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
            sock.settimeout(2)
            sock.connect(UART_WRITER_SOCK_PATH)
            sock.sendall(len(pkt).to_bytes(4, "big") + pkt)
    except OSError as e:
        print(f"⚠️ Failed to forward packet to uart-writer service: {e}", flush=True)
