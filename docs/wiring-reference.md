# Wiring Reference

This document collects the current known wiring and pin assignments for the ESP32-S3 firmware side and the Raspberry Pi companion side.

Primary source:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)

Related Raspberry Pi references:

- [lib/RPI4Scripts and Commands/UAart5Listener.py](../lib/RPI4Scripts%20and%20Commands/UAart5Listener.py)
- [lib/RPI4Scripts and Commands/heartbeat_sender.py](../lib/RPI4Scripts%20and%20Commands/heartbeat_sender.py)

## 1. ESP32-S3 Pin Assignments

### 1.1 UART And Handshake

| Function | ESP32-S3 Pin | Notes |
|---|---:|---|
| UART port | `UART_NUM_2` | main Raspberry Pi link |
| UART TX | `GPIO2` | ESP32 -> Pi data |
| UART RX | `GPIO1` | Pi -> ESP32 data |
| Pi data-ready input | `GPIO42` | Pi tells ESP32 UART data is ready |
| ESP32 data-ready output | `GPIO41` | ESP32 tells Pi UART data is ready |

### 1.2 I2C And Input Expander

| Function | ESP32-S3 Pin |
|---|---:|
| I2C SCL | `GPIO15` |
| I2C SDA | `GPIO16` |
| MCP interrupt | `GPIO18` |
| MCP enable | `GPIO17` |
| MCP address | `0x20` |

### 1.3 Relay Outputs

| Function | ESP32-S3 Pin |
|---|---:|
| Screen power relay | `GPIO13` |
| DAC power relay | `GPIO12` |
| Raspberry Pi power relay | `GPIO11` |
| Output stage power relay | `GPIO9` |
| Prototype DAC enable relay | `GPIO10` |
| General relay 1 | `GPIO47` |
| General relay 2 | `GPIO39` |

### 1.4 Indicator Outputs

| Function | ESP32-S3 Pin |
|---|---:|
| App active LED | `GPIO3` |
| App standby LED | `GPIO4` |
| Working status LED | `GPIO48` |
| Button LED PWM | `GPIO21` |
| Monitor brightness control | `GPIO43` |
| SPI LED data | `GPIO7` |
| SPI LED clock | `GPIO6` |
| SPI LED latch | `GPIO5` |

## 2. Raspberry Pi Side Wiring

### 2.1 Confirmed Script-Level Pin Usage

| Function | Pi BCM | Pi Physical Pin | Notes |
|---|---:|---:|---|
| ESP32 -> Pi data-ready input | 23 | 16 | listener waits on rising edge |
| Pi -> ESP32 data-ready output | 24 | 18 | sender script pulses this line |

### 2.2 UART5 Header Mapping

Using the standard `dtoverlay=uart5` mapping on a 40-pin Raspberry Pi header:

| Function | Pi BCM | Pi Physical Pin |
|---|---:|---:|
| UART5 TX | 12 | 32 |
| UART5 RX | 13 | 33 |

## 3. End-To-End Connection Table

| Function | Raspberry Pi Side | ESP32-S3 Side |
|---|---|---|
| Pi UART5 TX -> ESP32 UART RX | BCM12, pin 32 | GPIO1 |
| Pi UART5 RX <- ESP32 UART TX | BCM13, pin 33 | GPIO2 |
| ESP32 -> Pi data-ready | BCM23, pin 16 | GPIO41 |
| Pi -> ESP32 data-ready | BCM24, pin 18 | GPIO42 |
| Ground | Pi GND | ESP32 GND |

Important rule:

- UART TX always connects to the other side's RX.

## 4. Handshake Meaning

The design uses two extra GPIO lines in addition to UART.

### 4.1 ESP32 -> Pi Notification

- ESP32 writes a UART packet.
- ESP32 raises its data-ready output briefly.
- Pi listener wakes on BCM23 and reads from `/dev/ttyAMA5`.

### 4.2 Pi -> ESP32 Notification

- Pi writes a UART packet.
- Pi pulses BCM24.
- ESP32 sees GPIO42 rising edge and services UART RX.

This means the UART bytes and the ready notification are separate signals.

## 5. Pi Numbering Reminder

The Python scripts use:

- `GPIO.setmode(GPIO.BCM)`

That means values like `23` and `24` are BCM GPIO numbers, not physical header pin numbers.

## 6. Related Docs

- [docs/project-guide.md](./project-guide.md)
- [docs/protocol-reference.md](./protocol-reference.md)
- [docs/raspberry-pi-setup.md](./raspberry-pi-setup.md)