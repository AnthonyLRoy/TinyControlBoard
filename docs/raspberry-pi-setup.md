# Raspberry Pi Setup Reference

This document condenses the current Raspberry Pi side setup and companion scripts used by the project.

Primary source:

- [scripts/rpi/README.md](../scripts/rpi/README.md)

Companion scripts:

- [scripts/rpi/home/antho/UAart5Listener.py](../scripts/rpi/home/antho/UAart5Listener.py)
- [scripts/rpi/home/antho/heartbeat_sender.py](../scripts/rpi/home/antho/heartbeat_sender.py)

## 1. What Runs On The Pi

The Raspberry Pi side currently has two main Python processes:

- a UART5 listener that receives commands from the ESP32 and runs local commands,
- a heartbeat sender that periodically transmits a heartbeat packet back to the ESP32.

## 2. UART Listener Role

The listener script:

- opens `/dev/ttyAMA5`,
- waits on BCM GPIO23 for a rising edge,
- reads one 18-byte packet,
- verifies the checksum,
- dispatches the command to local handlers.

Current handled actions include:

- Raspberry Pi shutdown,
- playback next/previous,
- play/pause,
- stop,
- seek forward/back,
- rotary next/previous,
- meter toggle,
- cover view toggle.

Reference:

- [scripts/rpi/home/antho/UAart5Listener.py](../scripts/rpi/home/antho/UAart5Listener.py)

## 3. Heartbeat Sender Role

The heartbeat script:

- opens `/dev/ttyAMA5`,
- waits until the ESP32 data-ready line is low,
- sends a heartbeat packet every 10 seconds,
- pulses BCM GPIO24 to notify the ESP32 that data is waiting.

Reference:

- [scripts/rpi/home/antho/heartbeat_sender.py](../scripts/rpi/home/antho/heartbeat_sender.py)

## 4. Required Pi Configuration

The Pi setup docs currently require:

- `dtoverlay=uart5` in `/boot/firmware/config.txt`,
- Python serial support,
- `pigpiod`,
- systemd services for listener and heartbeat sender.

The config file included in the repo also shows `dtoverlay=uart5`.

Reference:

- [scripts/rpi/boot/firmware/config.txt](../scripts/rpi/boot/firmware/config.txt)

## 5. Service Summary

Current intended services:

| Service | Role |
|---|---|
| `uart_listener.service` | receives ESP32 commands |
| `heartbeat.service` | periodically sends heartbeat to ESP32 |
| `pigpiod` | GPIO support used by the Pi-side setup |

From the current setup notes, the service start commands are based on:

- `/usr/bin/python3 /home/antho/uart5_listener.py`
- `/usr/bin/python3 /home/antho/heartbeat_sender.py`

## 6. Important Pi File Locations

Current documented paths:

| Purpose | Path |
|---|---|
| UART5 overlay config | `/boot/firmware/config.txt` |
| listener script | `/home/antho/uart5_listener.py` |
| listener service | `/etc/systemd/system/uart_listener.service` |
| heartbeat service | `/etc/systemd/system/heartbeat.service` |
| pigpio daemon | `/usr/local/bin/pigpiod` |

## 7. Wiring Expectations On The Pi

Current script-level GPIO usage:

| Purpose | Pi BCM | Physical Pin |
|---|---:|---:|
| listener data-ready input | 23 | 16 |
| heartbeat data-ready output | 24 | 18 |

Typical UART5 mapping with `dtoverlay=uart5`:

| Purpose | Pi BCM | Physical Pin |
|---|---:|---:|
| UART5 TX | 12 | 32 |
| UART5 RX | 13 | 33 |

## 8. Operational Summary

The intended behavior is:

1. ESP32 sends a command to the Pi and pulses its ready line.
2. Pi listener wakes and executes the requested action.
3. Pi heartbeat sender continues to send heartbeat packets.
4. ESP32 uses those packets to decide whether the Pi is online, booted, or shut down.

## 9. Troubleshooting Notes

Current troubleshooting guidance in the repo includes:

- verify `dtoverlay=uart5`,
- confirm `/dev/ttyAMA5` exists,
- make sure no conflicting serial console is enabled,
- inspect `systemctl status` for the services,
- watch logs with `journalctl -u <service> -f`.

## 10. Related Docs

- [docs/project-guide.md](./project-guide.md)
- [docs/wiring-reference.md](./wiring-reference.md)
- [docs/protocol-reference.md](./protocol-reference.md)