# Raspberry Pi Interface Board

> **Superseded summary for the Pi4 Hat / PiHat board.** Use the full service guide at [PiHat/PiHat-EngineeringAndService.md](PiHat/PiHat-EngineeringAndService.md) and the action list at [PiHat/OpenQuestions.md](PiHat/OpenQuestions.md). The KiCad source reviewed for the full guide does **not** exactly match the assumptions in this older summary.

[Back to the Circuit Board Reference](../CircuitBoardReference.md) · [Service Guide](../../ServiceGuide.md)

A board that mates with a Raspberry Pi 40-pin header. It provides a filtered 5 V supply for the Pi and brings the I2S and I2C signals out to five signal connectors through 22 Ω series resistors. A few other GPIO lines are named for use elsewhere in the system.

## What It Does

- **5 V in:** J7 supplies +5 V to the Pi through header pins 2 and 4, with bulk and decoupling capacitors on the rail.
- **Digital audio out:** I2S data, word clock and bit clock go to three signal connectors through 22 Ω resistors.
- **I2C out:** SDA and SCL go to two more signal connectors through 22 Ω resistors.
- **System lines:** a handshake pair, a serial pair and a PWM monitor line are named on the header (see below).

## Connectors

| Ref | Part | Use |
|---|---|---|
| J3 | 70246-4001 (2 x 20) | Raspberry Pi 40-pin header |
| J7 | 1x03 socket | 5 V power input: pins 1 and 2 = +5 V, pin 3 = GND |
| J6 | 1725672 (4-pin) | Ground terminal, all four pins tied to GND |
| J4, J5, J8, J13 | Single pin | +5 V take-off pins |
| J14 | Single pin | Connected to GND2, labelled "5v pin" (see Notes) |
| 5 signal connectors | SIG, GND_1, GND_2 | One per signal below (refs J2, J9, J10, J11 and one more; ref positions not clear on the screenshot) |
| J1 | 2 x 20 "GPIO" header | No connections drawn on this sheet (see Notes) |

## Signal Outputs

Each signal passes through a 22 Ω series resistor to pin 1 (SIG) of its connector. Pins 2 and 3 are GND.

| Signal | Pi GPIO | Header pin | Resistor |
|---|---|---|---|
| I2S data out | GPIO21 / I2S DOUT | 40 | R1 |
| I2S word clock | GPIO19 / I2S LRCK | 35 | R2 |
| I2S bit clock | GPIO18 / I2S BCL | 12 | R3 |
| I2C clock | GPIO3 / SCL1 | 5 | R4 |
| I2C data | GPIO2 / SDA1 | 3 | R5 |

## Other Named Lines on J3

These lines are labelled on the header with no connection shown on this sheet.

| Signal | Pi GPIO | Header pin |
|---|---|---|
| ESP DRY | GPIO23 | 16 |
| RPI DRY | GPIO24 | 18 |
| Serial TX | GPIO12 | 32 |
| Serial RX | GPIO13 | 33 |
| Mon PWM | GPIO26 | 37 |

All other header pins (SPI, UART0, ID EEPROM, GPIO4, 5, 6, 7, 8, 16, 17, 20, 22, 25, 27 and others) are marked no-connect.

## Power

| Item | Detail |
|---|---|
| Input | J7 pins 1 and 2 = +5 V, pin 3 = GND |
| To Pi | +5 V on J3 pins 2 and 4 |
| 3.3 V | J3 pins 1 and 17 (from the Pi; only a power flag on this sheet) |
| GND | J3 pins 6, 9, 14, 20, 25, 30, 34, 39 |
| Bulk capacitance | C2, C3, C5, C6 (470 µF each, 1880 µF total) |
| Other capacitors | C4 (4.7 µF), C7 (0.1 µF) |
| Indicator | D1 with R6 (1 kΩ), lit when +5 V is present |

## Quick Checks

| Check | Expected |
|---|---|
| J7 pin 1 or 2 to pin 3 | +5 V |
| D1 | Lit |
| J3 pin 2 or 4 to GND | +5 V |
| J3 pin 1 or 17 to GND | +3.3 V (Pi running) |
| Signal pin on a connector | 3.3 V logic from the Pi when active |

## Quick Troubleshooting

| Symptom | Check |
|---|---|
| D1 off | Supply on J7, polarity, then R6 and D1 |
| Pi does not power up | +5 V at J3 pins 2 and 4, supply current capability, connector seating on the Pi |
| No I2S or I2C signal at a connector | Pi software configuration, then the 22 Ω resistor, then the connector |
| I2S output distorted or missing | Check all three I2S signals (clock, word clock, data) are reaching their connectors |
| Rail droops or Pi resets | Supply wiring and capacity, bulk capacitors C2, C3, C5, C6 |
| Noisy signals | Ground connections at pins 2 and 3 of each signal connector, GND link to J6 |

## Notes

- **Powering the Pi from the 40-pin header** bypasses the Pi's own input protection. Use a supply of the correct voltage and rating, and do not connect USB power at the same time as J7.
- **J14** is labelled "5v pin" but is connected to GND2, which has no other connection on this sheet. Check whether it is a mislabelled ground pin.
- **J1** shows no wiring on this sheet. It may be a footprint-only or unfinished part; confirm in the KiCad project.
- **I2C lines** have only 22 Ω series resistors on this sheet and no pull-ups. The Pi has on-board pull-ups on GPIO2 and GPIO3, but check the downstream device if I2C is unreliable.
- Pin and net details were read from a screenshot. Confirm against the KiCad schematic before using them for repairs.
