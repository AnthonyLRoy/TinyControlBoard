# Wiring Reference

This document collects the current known wiring and pin assignments for the ESP32-S3 firmware side and the Raspberry Pi companion side.

It also mirrors the button-to-command and button-to-SPI-LED mapping so the firmware view can be compared directly against the physical schematic blocks for button inputs and button LED drive.

Current hardware schematic image:

![TinyControlBoard schematic](./schema.png)

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

### 1.5 Button Inputs And SPI LED Drive

From the firmware side, the button path is split into two hardware blocks:

- button inputs are read through the I2C GPIO expander and reported as button indices `0..15`,
- button LEDs are driven by a 16-bit SPI-backed shift-register state where button index `n` maps to bit `n`.

This is the same logical mapping documented in [docs/button-command-map.md](./button-command-map.md), placed here so it can be read alongside the physical circuit diagram.

| Button Index | Button Name | Input Source | SPI LED Register Bit | SPI Register Value From Idle | SPI TX Bytes | Command ID | Command Name | Notes |
|---|---|---|---|---|---|---|---|---|
| 0 | Power | MCP input index 0 | bit 0 | `0x0001` | `0x01 0x00` | `0x0001` | `CMD_SYS_POWER` | timed action, command emitted on release |
| 1 | Previous Track | MCP input index 1 | bit 1 | `0x0002` | `0x02 0x00` | `0x0101` | `CMD_PREVIOUS_TRACK` | UART command |
| 2 | Next Track | MCP input index 2 | bit 2 | `0x0004` | `0x04 0x00` | `0x0100` | `CMD_NEXT_TRACK` | UART command |
| 3 | Skip Forward | MCP input index 3 | bit 3 | `0x0008` | `0x08 0x00` | `0x0104` | `CMD_SKIP_FORWARD` | UART command |
| 4 | Skip Back | MCP input index 4 | bit 4 | `0x0010` | `0x10 0x00` | `0x0105` | `CMD_SKIP_BACK` | UART command |
| 5 | Play/Pause | MCP input index 5 | bit 5 | `0x0020` | `0x20 0x00` | `0x0102` | `CMD_PLAY_PAUSE` | UART command |
| 6 | Stop | MCP input index 6 | bit 6 | `0x0040` | `0x40 0x00` | `0x0103` | `CMD_STOP_TRACK` | UART command |
| 7 | Cover | MCP input index 7 | bit 7 | `0x0080` | `0x80 0x00` | `0x0117` or `0x0118` | `CMD_COVER_VIEW_ON` or `CMD_COVER_VIEW_OFF` | normalized to `CMD_TOGGLE_COVER_VIEW` on UART |
| 8 | Next Menu | MCP input index 8 | bit 8 | `0x0100` | `0x00 0x01` | `0x0107` | `CMD_NEXT_MENU_ITEM` | UART command |
| 9 | Menu Select | MCP input index 9 | bit 9 | `0x0200` | `0x00 0x02` | `0x0108` | `CMD_ITEM_SELECT` | UART command |
| 10 | Toggle DAC | MCP input index 10 | bit 10 | `0x0400` | `0x00 0x04` | `0x010A` or `0x010F` | `CMD_TOGGLE_DAC_ON` or `CMD_TOGGLE_DAC_OFF` | local relay control |
| 11 | Toggle Display | MCP input index 11 | bit 11 | `0x0800` | `0x00 0x08` | `0x010B` or `0x010E` | `CMD_DISPLAY_OFF` or `CMD_DISPLAY_ON` | normalized to `CMD_TOGGLE_DISPLAY` on UART |
| 12 | Toggle Meter | MCP input index 12 | bit 12 | `0x1000` | `0x00 0x10` | `0x010C` or `0x010D` | `CMD_TOGGLE_METER_ON` or `CMD_TOGGLE_METER_OFF` | normalized to `CMD_TOGGLE_METER` on UART |
| 13 | Rotary Left | MCP input index 13 | none in current rotary path | n/a | n/a | `0x0112` | `CMD_ROTARY_ACTION` | sent with param `0`, no SPI LED write |
| 14 | Rotary Right | MCP input index 14 | none in current rotary path | n/a | n/a | `0x0112` | `CMD_ROTARY_ACTION` | sent with param `1`, no SPI LED write |
| 15 | Cycle Brightness | MCP input index 15 | bit 15 | `0x8000` | `0x00 0x80` | `0x0116` | `CMD_CYCLE_BRIGHTNESS` | local brightness control |

Important hardware interpretation:

- the SPI LED driver shifts the 16-bit LED register low byte first, then high byte,
- the values above assume an idle LED register before the button is pressed,
- if another LED is already latched on, the transmitted SPI value is the OR-combination of active button bits,
- rotary movement currently bypasses `setLed()` and therefore does not light a corresponding SPI LED bit.

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

- [docs/button-command-map.md](./button-command-map.md)
- [docs/project-guide.md](./project-guide.md)
- [docs/protocol-reference.md](./protocol-reference.md)
- [docs/raspberry-pi-setup.md](./raspberry-pi-setup.md)