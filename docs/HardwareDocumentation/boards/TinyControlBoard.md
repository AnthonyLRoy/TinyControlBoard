# Tiny Control Board for Raspberry Pi 4

[Back to the Circuit Board Reference](../CircuitBoardReference.md) · [Service Guide](../../ServiceGuide.md)

> **Detailed engineering and service document:** [TinyControlBoard-EngineeringAndService.md](TinyControlBoard/TinyControlBoard-EngineeringAndService.md) ([PDF](TinyControlBoard/TinyControlBoard-EngineeringAndService.pdf)), with [open questions](TinyControlBoard/OpenQuestions.md). It supersedes this summary where they differ (the ESP32 is IC6, not IC8; current net names are `rpi4_data_ready`, `esp_ir_sent`, `rpi_state`).

<img src="../../../user-manual-media/media/service-guide/control-board-schematic.png" style="width:4.43092in;height:3.08093in" />

<img src="../../../user-manual-media/media/service-guide/control-board-pcb-layout.png" style="width:4.17919in;height:3.71803in" />

Control board for the RPi 4 Streamer DAC (`tinyControlBoard.kicad_sch`, KiCad 8.0.0, 2024-08-08).

## What It Does

An ESP32-S3 handles the housekeeping for the Raspberry Pi:

- Reads 16 debounced front-panel buttons.
- Drives 16 button LEDs with adjustable brightness.
- Switches power to the Pi, DAC, screen and other boards through relay control lines.
- Talks to the Pi over serial, with two handshake lines.

## Block Overview

| Block | Part | Function |
|---|---|---|
| MCU | IC8 ESP32-S3-WROOM-1U-N16 | Main controller |
| Debouncers | IC2, IC3 MAX6818 | Debounce buttons 1-8 and 9-16 |
| I/O expander | IC5 MCP23018 | Reads the 16 button signals over I2C |
| LED driver | IC1 STP16CPC26 | 16 constant-current LED outputs; VR1 sets brightness |
| Level shifter | IC7 TXS0108E | 3.3 V to 5 V for the relay/power control lines |
| 3.3 V regulator | IC4 LM3940-3.3 | +5V to +3V3 |
| USB-C | J1 (ESD: U1 USBLC6-2SC6) | Programming, testing, backup 5 V |

## Power

- **Source select (S1):** position 1 = USB 5 V, position 3 = external supply (J8). Output is the +5V rail.
- **+3V3** comes from IC4 and powers the MCU, debouncers, expander, LED driver and level shifter (A side).
- **+5V** powers the button LEDs, level shifter (B side), and the 5 V pins on J3 and J6.
- **Indicators:** D2 green = 3V3 present, D3 red = 5 V present.
- **Status LED:** D1 green blinks when the ESP32 software is running.

## Connectors

| Ref | Use |
|---|---|
| J1 | USB-C for programming and testing |
| J3 (2x20) | Buttons 1-16, LEDs 1-16, on/standby LEDs. Buttons return to GND pins; LED anodes go to the +5V pins |
| J6 (2x20) | Pi serial (Tx/Rx), debug serial, handshake lines, relay/power outputs, 5 V and GND |
| J8 | External supply input |

#### J6 Signals

| Pin | Signal |
|---|---|
| 3 / 4 | Serial Tx / Rx |
| 5 / 6 | Debug Tx / Rx |
| 9 | `rpi_data_ready` |
| 11 | `msg_ir_wait` |
| 12 | `rpi_status` |
| 21 | `relay_pwr_1` out |
| 23 | `dac_on` out |
| 25 | `screen_on` out |
| 27 | `dac_power` out |
| 29 | `rpi_power` out |
| 31 | `protodac_on` out |
| 33 | `output_stage` out |
| 35 | `relay_pwr_2` out |
| 1, 2, 39, 40 | +5V |

## Controls and Adjustments

- **S2 RESET:** resets the ESP32.
- **S3 BOOT:** hold during reset or power-up to enter the ESP32 download mode.
- **VR1:** trimmer that sets the brightness of all button LEDs.

## Test Points

TP1-TP10 cover the LED-driver serial lines, expander I2C and reset, debug UART and interrupt lines. See the board silkscreen for each label.

## Quick Troubleshooting

| Symptom | Check |
|---|---|
| No power LEDs | S1 position, supply at J8 or USB, then IC4 output on +3V3 |
| 5 V LED on, 3.3 V LED off | IC4 and its 33 µF output capacitor |
| D1 not blinking | ESP32 not running; try S2 reset, check debug serial output |
| Cannot program the ESP32 | Use S3 BOOT with S2 reset; check the USB cable and J1 |
| One button not responding | Wiring on J3, then the matching debouncer input and output |
| All buttons dead | I2C lines (`mcp_clk`, `mcp_sda`), expander reset, +3V3 |
| LEDs dark or dim | VR1 setting, LED driver supply, +5V at J3 |
| Relay/power outputs inactive | Level shifter supplies (3V3 and 5V), then the signal at J6 |
| Pi and ESP32 not communicating | Serial wiring on J6 pins 3/4, then `msg_ir_wait` and `rpi_data_ready` |


