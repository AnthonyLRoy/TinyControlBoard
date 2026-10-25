# Tiny Control Board – unresolved questions, discrepancies and assumptions

Companion to [TinyControlBoard-EngineeringAndService.md](TinyControlBoard-EngineeringAndService.md). Source: KiCad 8.0 project dated 2024-08-08. Nothing was measured on hardware.

## A. Discrepancies in the source files

| # | Item | Detail | Suggested action |
|---|---|---|---|
| A1 | R1, R2 value | BOM says "100R"; MPN ERJ-H3EF1200V is 120 Ω | Check the fitted parts; fix the BOM or the part number |
| A2 | J3 pins 32 / 34 | led_o_16 on pin 32 and led_o_15 on pin 34 (others run in order) | Confirm against the front-panel loom |
| A3 | IC7 B1 net name | `gen_relay_1_out` (J6-35) carries `relay_gen_2`; `relay_gen_1_out` (J6-21) carries `relay_gen_1` | Confirm the intended names with the relay board documentation |
| A4 | Schematic image text | Some text shows "IC8" for the ESP32; the netlist says IC6 | Update the schematic text |
| A5 | Earlier summary document | Wrong refdes (IC8) and old net names (`rpi_data_ready`, `msg_ir_wait`, `rpi_status`, `relay_pwr_*`); says "33 µF" (C1 is 0.33 µF) | Corrected in the new document; update or retire the old page |
| A6 | Stray silkscreen | Free text "R10" at about (72.35, 86.7) | Tidy up in the next revision |
| A7 | ERC/DRC | 31 ERC warnings (library mismatch, footprint links), 40 DRC silkscreen-overlap warnings | Re-run ERC with the original libraries; the ERC warnings may be a result of the library environment used here |
| A8 | Revision | No revision field in the title block | Add one |

## B. Design risks needing review or measurement

| # | Risk | Basis |
|---|---|---|
| B1 | LM3940 output capacitance ≈ 10.9 µF all-ceramic versus ≥ 33 µF with ESR window | Datasheet, netlist; scope +3V3 |
| B2 | No fuse, reverse-polarity or OR-ing protection on J8 / USB | Netlist |
| B3 | VR1 is the only R-EXT resistor; low setting may over-drive the STP16CPC26 | Inferred; datasheet not retrieved |
| B4 | I²C SDA/SCL have no external pull-ups; 400 kHz marginal | Netlist; MCP23018 datasheet |
| B5 | MCP23018 RESET has no pull-up until IO17 is driven | Netlist |
| B6 | C10 on BOOT; designer's own note about boot failure | Schematic note |
| B7 | IO3 (strapping pin, no internal pull) drives the on_led circuit | ESP32 datasheet, netlist |
| B8 | TXS0108E OE is pulled up (datasheet recommends pull-down); J6 output states during boot are not established | Datasheet, netlist |
| B9 | +5V LED current runs on 0.5–1 mm traces | PCB |
| B10 | LM3940 dissipation depends on the real 3.3 V load (not known) | Calculated |

## C. Information not available

- Real current consumption, LED current per output, operating temperature range.
- Meaning of "pressed" logic level and debounce time for MAX6818 (datasheet not retrieved).
- STP16CPC26 R-EXT equation and limits; USBLC6-2SC6 parameters (datasheets not retrieved).
- Which S1 slide direction selects USB or J8.
- Whether the J3 pin-1 row is the top or bottom row, confirmed from pad data.
- Direction of J6-12 `rpi_state` (assumed ESP → Pi).
- MCP23018 address 0x20 is derived from the divider; an I²C scan is needed.
- Firmware behaviour: pin defaults, I²C speed, PWM frequency, heartbeat rate.
- Relay/DAC power-sequencing order and timing.

## D. Assumptions made

- Netlist and PCB from `kicad-cli 10.0.6` are an accurate representation of the 8.0 files.
- "Top view" images are a 3D render with component labels added from the PCB positions.
- Part values come from `Bom.csv`; where they conflict with the part number this is flagged, not resolved.
