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
| 15 | Cycle Brightness | MCP input index 15 | bit 14 | `0x4000` | `0x00 0x40` | `0x0116` | `CMD_CYCLE_BRIGHTNESS` | local brightness control |

Important hardware interpretation:

- the SPI LED driver accepts a zero-based `ledIndex` (0–15) and has no knowledge of button IDs,
- `ControlBoard` maps button IDs to LED indices by subtracting 1: `ledIndex = buttonId - 1`,
- button 0 (power) is skipped and does not drive any SPI LED bit,
- the SPI LED driver shifts the 16-bit LED register low byte first, then high byte,
- the values above assume an idle LED register before the button is pressed,
- if another LED is already latched on, the transmitted SPI value is the OR-combination of active LED bits,
- rotary movement currently bypasses `setLed()` and therefore does not light a corresponding SPI LED bit.

### 1.6 Button Pin To LED Relationship

Looking at the schematic and the firmware together, the relationship is logical rather than a direct hardwired button-to-LED coupling inside the board logic:

- each button input arrives on a numbered `btn_in_x` net through the debouncer and MCP23016/23018 input expander,
- the ESP32 converts that numbered input into a firmware button index,
- `ControlBoard` maps the button index to an LED index by subtracting 1 (`ledIndex = buttonId - 1`),
- button 0 (power) is skipped — it has no corresponding LED,
- the SPI shift register then drives the corresponding LED output.

So the normal pattern is:

- `btn_in_1` drives firmware button index `0` (power), which has no LED,
- `btn_in_2` drives firmware button index `1`, which lights SPI LED bit `0`,
- `btn_in_3` drives firmware button index `2`, which lights SPI LED bit `1`,
- and so on.

The main exception is rotary movement: those inputs are still read as indices `13` and `14`, but the current firmware does not call `setLed()` for rotary events, so no SPI LED bit is written for them.

| Schematic Input Net | Firmware Button Index | Firmware Name | Command Name | LED Relationship |
|---|---:|---|---|---|
| `btn_in_1` | 0 | `kPower` | `CMD_SYS_POWER` | no LED (power button skipped) |
| `btn_in_2` | 1 | `kPrevTrack` | `CMD_PREVIOUS_TRACK` | lights SPI bit 0, register `0x0001`, LED output 1 |
| `btn_in_3` | 2 | `kNextTrack` | `CMD_NEXT_TRACK` | lights SPI bit 1, register `0x0002`, LED output 2 |
| `btn_in_4` | 3 | `kSkipForward` | `CMD_SKIP_FORWARD` | lights SPI bit 2, register `0x0004`, LED output 3 |
| `btn_in_5` | 4 | `kSkipBack` | `CMD_SKIP_BACK` | lights SPI bit 3, register `0x0008`, LED output 4 |
| `btn_in_6` | 5 | `kPlayPause` | `CMD_PLAY_PAUSE` | lights SPI bit 4, register `0x0010`, LED output 5 |
| `btn_in_7` | 6 | `kStop` | `CMD_STOP_TRACK` | lights SPI bit 5, register `0x0020`, LED output 6 |
| `btn_in_8` | 7 | `kCover` | `CMD_COVER_VIEW_ON` or `CMD_COVER_VIEW_OFF` | lights SPI bit 6, register `0x0040`, LED output 7 |
| `btn_in_9` | 8 | `kNextMenu` | `CMD_NEXT_MENU_ITEM` | lights SPI bit 7, register `0x0080`, LED output 8 |
| `btn_in_10` | 9 | `kMenuSelect` | `CMD_ITEM_SELECT` | lights SPI bit 8, register `0x0100`, LED output 9 |
| `btn_in_11` | 10 | `kToggleDac` | `CMD_TOGGLE_DAC_ON` or `CMD_TOGGLE_DAC_OFF` | lights SPI bit 9, register `0x0200`, LED output 10 |
| `btn_in_12` | 11 | `kToggleDisplay` | `CMD_DISPLAY_OFF` or `CMD_DISPLAY_ON` | lights SPI bit 10, register `0x0400`, LED output 11 |
| `btn_in_13` | 12 | `kToggleMeter` | `CMD_TOGGLE_METER_ON` or `CMD_TOGGLE_METER_OFF` | lights SPI bit 11, register `0x0800`, LED output 12 |
| `btn_in_14` | 13 | `kRotaryEventLeft` | `CMD_ROTARY_ACTION` with param `0` | no LED update in current firmware |
| `btn_in_15` | 14 | `kRotaryEventRight` | `CMD_ROTARY_ACTION` with param `1` | no LED update in current firmware |
| `btn_in_16` | 15 | `kCycleBrightness` | `CMD_CYCLE_BRIGHTNESS` | lights SPI bit 14, register `0x4000`, LED output 15 |

In short: button 0 (power) has no LED. For all other push buttons, input number `N` maps to LED bit `N-1`. Firmware stores the button as zero-based index and subtracts 1 to get the LED index. Rotary left and right break that pattern because they are input-only events in the current code path.

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