# Circuit Board Reference

[Back to the Service Guide](../ServiceGuide.md) · [DAC and audio subsystem](DacSubsystem.md) · [Power supply subsystem](PowerSupplySubsystem.md) · [System wiring and commissioning](SystemWiring.md)

This reference collects the existing board descriptions, connector information, test points, and troubleshooting notes. Treat details marked uncertain as unverified; check the current KiCad schematics and component datasheets before wiring or servicing a board.

## Tiny Control Board for Raspberry Pi 4

<img src="../../user-manual-media/media/service-guide/control-board-schematic.png" style="width:4.43092in;height:3.08093in" />

<img src="../../user-manual-media/media/service-guide/control-board-pcb-layout.png" style="width:4.17919in;height:3.71803in" />

Control board for the RPi 4 Streamer DAC (`tinyControlBoard.kicad_sch`, KiCad 8.0.0, 2024-08-08).

### What It Does

An ESP32-S3 handles the housekeeping for the Raspberry Pi:

- Reads 16 debounced front-panel buttons.
- Drives 16 button LEDs with adjustable brightness.
- Switches power to the Pi, DAC, screen and other boards through relay control lines.
- Talks to the Pi over serial, with two handshake lines.

### Block Overview

| Block | Part | Function |
|---|---|---|
| MCU | IC8 ESP32-S3-WROOM-1U-N16 | Main controller |
| Debouncers | IC2, IC3 MAX6818 | Debounce buttons 1-8 and 9-16 |
| I/O expander | IC5 MCP23018 | Reads the 16 button signals over I2C |
| LED driver | IC1 STP16CPC26 | 16 constant-current LED outputs; VR1 sets brightness |
| Level shifter | IC7 TXS0108E | 3.3 V to 5 V for the relay/power control lines |
| 3.3 V regulator | IC4 LM3940-3.3 | +5V to +3V3 |
| USB-C | J1 (ESD: U1 USBLC6-2SC6) | Programming, testing, backup 5 V |

### Power

- **Source select (S1):** position 1 = USB 5 V, position 3 = external supply (J8). Output is the +5V rail.
- **+3V3** comes from IC4 and powers the MCU, debouncers, expander, LED driver and level shifter (A side).
- **+5V** powers the button LEDs, level shifter (B side), and the 5 V pins on J3 and J6.
- **Indicators:** D2 green = 3V3 present, D3 red = 5 V present.
- **Status LED:** D1 green blinks when the ESP32 software is running.

### Connectors

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

### Controls and Adjustments

- **S2 RESET:** resets the ESP32.
- **S3 BOOT:** hold during reset or power-up to enter the ESP32 download mode.
- **VR1:** trimmer that sets the brightness of all button LEDs.

### Test Points

TP1-TP10 cover the LED-driver serial lines, expander I2C and reset, debug UART and interrupt lines. See the board silkscreen for each label.

### Quick Troubleshooting

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

## Raspberry Pi Interface Board

A board that mates with a Raspberry Pi 40-pin header. It provides a filtered 5 V supply for the Pi and brings the I2S and I2C signals out to five signal connectors through 22 Ω series resistors. A few other GPIO lines are named for use elsewhere in the system.

### What It Does

- **5 V in:** J7 supplies +5 V to the Pi through header pins 2 and 4, with bulk and decoupling capacitors on the rail.
- **Digital audio out:** I2S data, word clock and bit clock go to three signal connectors through 22 Ω resistors.
- **I2C out:** SDA and SCL go to two more signal connectors through 22 Ω resistors.
- **System lines:** a handshake pair, a serial pair and a PWM monitor line are named on the header (see below).

### Connectors

| Ref | Part | Use |
|---|---|---|
| J3 | 70246-4001 (2 x 20) | Raspberry Pi 40-pin header |
| J7 | 1x03 socket | 5 V power input: pins 1 and 2 = +5 V, pin 3 = GND |
| J6 | 1725672 (4-pin) | Ground terminal, all four pins tied to GND |
| J4, J5, J8, J13 | Single pin | +5 V take-off pins |
| J14 | Single pin | Connected to GND2, labelled "5v pin" (see Notes) |
| 5 signal connectors | SIG, GND_1, GND_2 | One per signal below (refs J2, J9, J10, J11 and one more; ref positions not clear on the screenshot) |
| J1 | 2 x 20 "GPIO" header | No connections drawn on this sheet (see Notes) |

### Signal Outputs

Each signal passes through a 22 Ω series resistor to pin 1 (SIG) of its connector. Pins 2 and 3 are GND.

| Signal | Pi GPIO | Header pin | Resistor |
|---|---|---|---|
| I2S data out | GPIO21 / I2S DOUT | 40 | R1 |
| I2S word clock | GPIO19 / I2S LRCK | 35 | R2 |
| I2S bit clock | GPIO18 / I2S BCL | 12 | R3 |
| I2C clock | GPIO3 / SCL1 | 5 | R4 |
| I2C data | GPIO2 / SDA1 | 3 | R5 |

### Other Named Lines on J3

These lines are labelled on the header with no connection shown on this sheet.

| Signal | Pi GPIO | Header pin |
|---|---|---|
| ESP DRY | GPIO23 | 16 |
| RPI DRY | GPIO24 | 18 |
| Serial TX | GPIO12 | 32 |
| Serial RX | GPIO13 | 33 |
| Mon PWM | GPIO26 | 37 |

All other header pins (SPI, UART0, ID EEPROM, GPIO4, 5, 6, 7, 8, 16, 17, 20, 22, 25, 27 and others) are marked no-connect.

### Power

| Item | Detail |
|---|---|
| Input | J7 pins 1 and 2 = +5 V, pin 3 = GND |
| To Pi | +5 V on J3 pins 2 and 4 |
| 3.3 V | J3 pins 1 and 17 (from the Pi; only a power flag on this sheet) |
| GND | J3 pins 6, 9, 14, 20, 25, 30, 34, 39 |
| Bulk capacitance | C2, C3, C5, C6 (470 µF each, 1880 µF total) |
| Other capacitors | C4 (4.7 µF), C7 (0.1 µF) |
| Indicator | D1 with R6 (1 kΩ), lit when +5 V is present |

### Quick Checks

| Check | Expected |
|---|---|
| J7 pin 1 or 2 to pin 3 | +5 V |
| D1 | Lit |
| J3 pin 2 or 4 to GND | +5 V |
| J3 pin 1 or 17 to GND | +3.3 V (Pi running) |
| Signal pin on a connector | 3.3 V logic from the Pi when active |

### Quick Troubleshooting

| Symptom | Check |
|---|---|
| D1 off | Supply on J7, polarity, then R6 and D1 |
| Pi does not power up | +5 V at J3 pins 2 and 4, supply current capability, connector seating on the Pi |
| No I2S or I2C signal at a connector | Pi software configuration, then the 22 Ω resistor, then the connector |
| I2S output distorted or missing | Check all three I2S signals (clock, word clock, data) are reaching their connectors |
| Rail droops or Pi resets | Supply wiring and capacity, bulk capacitors C2, C3, C5, C6 |
| Noisy signals | Ground connections at pins 2 and 3 of each signal connector, GND link to J6 |

### Notes

- **Powering the Pi from the 40-pin header** bypasses the Pi's own input protection. Use a supply of the correct voltage and rating, and do not connect USB power at the same time as J7.
- **J14** is labelled "5v pin" but is connected to GND2, which has no other connection on this sheet. Check whether it is a mislabelled ground pin.
- **J1** shows no wiring on this sheet. It may be a footprint-only or unfinished part; confirm in the KiCad project.
- **I2C lines** have only 22 Ω series resistors on this sheet and no pull-ups. The Pi has on-board pull-ups on GPIO2 and GPIO3, but check the downstream device if I2C is unreliable.
- Pin and net details were read from a screenshot. Confirm against the KiCad schematic before using them for repairs.

## Power Relay Board

<img src="../../user-manual-media/media/service-guide/power-relay-board-schematic.png" style="width:6.26806in;height:4.86319in" />

<img src="../../user-manual-media/media/service-guide/power-relay-board-layout.png" style="width:6.26806in;height:3.94653in" />

## 6-Channel Relay Board

A six-relay switching board. Logic-level control inputs drive a ULN2003LV transistor array, which energises six 5 V relays. Each relay brings out one switched contact pair on a screw terminal.

### What It Does

**Signal path:** J2 (control inputs) → IC1 ULN2003LVDR → relay coils K1–K6 → contact pairs on J4 and J5

An input going high turns on the matching driver output, which pulls the relay coil to GND and closes the relay.

### Connectors

| Ref | Use | Pins |
|---|---|---|
| J1 | Power input | 1 = GND, 2 and 3 = supply (+5 V, see Notes) |
| J2 | Control inputs | 1 to 6 = Relay 1 In to Relay 6 In |
| J4 | Relay contacts, K1 to K3 | 1-2 = K1, 3-4 = K2, 5-6 = K3 |
| J5 | Relay contacts, K4 to K6 | 1-2 = K4, 3-4 = K5, 5-6 = K6 |

All connectors are screw terminals.

### Input-to-Relay Mapping

**The numbering is reversed between the inputs and the relays.**

| J2 input | IC1 input | IC1 output | Relay | Contacts at |
|---|---|---|---|---|
| Relay 6 In (pin 6) | IN1 | OUT1 | K1 | J4 pins 1-2 |
| Relay 5 In (pin 5) | IN2 | OUT2 | K2 | J4 pins 3-4 |
| Relay 4 In (pin 4) | IN3 | OUT3 | K3 | J4 pins 5-6 |
| Relay 3 In (pin 3) | IN4 | OUT4 | K4 | J5 pins 1-2 |
| Relay 2 In (pin 2) | IN5 | OUT5 | K5 | J5 pins 3-4 |
| Relay 1 In (pin 1) | IN6 | OUT6 | K6 | J5 pins 5-6 |

IN7 and OUT7 are not used.

### Components

| Ref | Part | Function |
|---|---|---|
| IC1 | ULN2003LVDR | 7-channel low-voltage Darlington driver; 6 channels used |
| K1–K6 | EE2-5NU | 5 V coil signal relays |
| C1 | 100 nF | Decoupling on the supply rail |
| R1 + D1 | 300 Ω + LED | Power-on indicator |

#### Relay Wiring

- **Coil:** pin 1 goes to the supply rail and pin 8 to the driver output.
- **Contacts:** only pins 5 and 6 of each relay are brought out, so each relay gives one switched contact pair. Pin 7 and pins 2 to 4 are unused.
- IC1 pin 9 (COM) is tied to the supply rail, which connects the driver's internal flyback diodes to the coil supply.

### Quick Checks

| Check | Expected |
|---|---|
| J1 pin 2 or 3 to pin 1 | Supply voltage (+5 V) |
| D1 | Lit when powered |
| Relay coil (pin 1 to pin 8) with input high | About supply voltage, relay clicks |
| Relay coil with input low | 0 V across the coil |

### Quick Troubleshooting

| Symptom | Check |
|---|---|
| D1 off | Supply on J1, polarity, then R1 and D1 |
| No relay operates | Supply on the relay rail, IC1 pin 8 (GND) and pin 9 (COM) |
| One relay does not operate | Input signal on J2, then IC1 input and output, then the coil |
| Wrong relay switches | Input numbering is reversed (see mapping table) |
| Relay clicks but no contact change | Wiring on J4 or J5, then the relay contacts |
| Relay stuck on | Input pin held high, shorted IC1 output, or damaged relay |
| Intermittent resets or noise | C1 and the supply rail |

### Notes

- The supply voltage is not labelled in the schematic. The EE2-5NU coils are 5 V, so it should be +5 V. Confirm before connecting.
- The reversed numbering between J2 and the relays is easy to miss. Check which relay you are testing.
- Whether the contact on pin 5 is normally open or normally closed depends on the relay datasheet. Confirm before wiring the load.
- Pin and net details were read from a screenshot. Confirm against the KiCad schematic before using them for repairs.
