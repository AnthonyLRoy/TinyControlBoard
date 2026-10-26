# Dac_output_ Relay

![3D board render](<board-render.png>)

## Overview

| Item | Details |
|---|---|
| MCU / main IC | No MCU. IC1 is a ULN2003LVDR relay driver. |
| Supply | The schematic specifies 5 V DC at J3: pins 2 and 3 are `+5V`, pin 1 is `GND2`. No current rating is specified. |
| Programming / debug | None shown. J2 is an `Earth` connection, not a programming or debug header. |
| Source files | `Relay OUT Signal switch board.kicad_pro`, `Relay OUT Signal switch board.kicad_sch`, and `Relay OUT Signal switch board.kicad_pcb`. The supplied BOM at `production/bom.csv` is included as [BOM.csv](<BOM.csv>). |

The schematic describes switching two balanced stereo inputs and two single-ended stereo inputs to balanced and single-ended outputs. It notes that all lines are switched together, unless testing.

## Schematic

![Schematic, sheet 1](<schematic/schematic-01.png>)

The project has one schematic sheet. Open the exported [PDF](<schematic/Relay OUT Signal switch board.pdf>) or [SVG](<schematic/Relay OUT Signal switch board.svg>).

## Functional blocks

The block names below follow the schematic. The relay sub-block titles are shown beside the relay symbols.

| Block | Parts | Description |
|---|---|---|
| Signal inputs | J10, J12-J16 | Two single-ended stereo input connectors and two balanced stereo input connectors. |
| Switching | K1-K3 | K1 switches balanced left, K2 switches balanced right, and K3 switches the single-ended signals. The schematic note says all lines are switched together, unless testing. |
| Signal outputs | J17-J19 | Balanced right, balanced left, and single-ended output connectors. |
| Power | J3, IC1, C1, D1, D2, R1, R2 | 5 V DC input and distribution, relay-driver connections, C1 (value `0.1`), and two resistor/LED branches as drawn. |
| Signal Control | J1, J2, R3 | J1's shared relay-driver input, the `Earth` connector, and the 10R resistor between `sig ground` and `GND2`. |

## Connector pinouts

Pin numbers and signal labels are taken from the schematic. Local labels are written as displayed; KiCad's exported netlist prefixes sheet-local net names with `/`.

### J1 — 1725669, 3-pin

| Pin | Schematic net | Connection |
|---:|---|---|
| 1 | `GND2` | Ground |
| 2 | `Net-(D2-A)` | Shared net with pin 3, D2 pin 2, and IC1 input pins 1-5 |
| 3 | `Net-(D2-A)` | Shared net with pin 2, D2 pin 2, and IC1 input pins 1-5 |

### J2 — Conn_01x01_Socket, 1-pin

| Pin | Schematic net | Connection |
|---:|---|---|
| 1 | `Earth` | Earth |

### J3 — 1725669, 3-pin power connector

| Pin | Schematic net | Connection |
|---:|---|---|
| 1 | `GND2` | Ground |
| 2 | `+5V` | 5 V DC |
| 3 | `+5V` | 5 V DC, shared with pin 2 |

### J10 — 1725669, single-ended input 1

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `SE_1_L` | Input 1, left |
| 2 | `sig ground` | Signal ground |
| 3 | `SE_1_R` | Input 1, right |

### J12 — 1725669, single-ended input 2

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `SE_2_L` | Input 2, left |
| 2 | `sig ground` | Signal ground |
| 3 | `SE_2_R` | Input 2, right |

### J13 — 1725669, balanced input 2 left

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `inp_2_bal_L_-` | Input 2, left negative |
| 2 | `sig ground` | Signal ground |
| 3 | `inp_2_bal_L_+` | Input 2, left positive |

### J14 — 1725669, balanced input 2 right

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `inp_2_bal_R_-` | Input 2, right negative |
| 2 | `sig ground` | Signal ground |
| 3 | `inp_2_bal_R_+` | Input 2, right positive |

### J15 — 1725669, balanced input 1 left

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `inp_1_bal_L_-` | Input 1, left negative |
| 2 | `sig ground` | Signal ground |
| 3 | `inp_1_bal_L_+` | Input 1, left positive |

### J16 — 1725669, balanced input 1 right

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `inp_1_bal_R_-` | Input 1, right negative |
| 2 | `sig ground` | Signal ground |
| 3 | `inp_1_bal_R_+` | Input 1, right positive |

### J17 — 1725669, balanced right output

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `out_R_+` | Right positive |
| 2 | `sig ground` | Signal ground |
| 3 | `out_R_-` | Right negative |

### J18 — 1725669, balanced left output

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `out_L_+` | Left positive |
| 2 | `sig ground` | Signal ground |
| 3 | `out_L_-` | Left negative |

### J19 — 1725669, single-ended output

| Pin | Schematic net | Function |
|---:|---|---|
| 1 | `SE_OUT_R` | Right output |
| 2 | `sig ground` | Signal ground |
| 3 | `SE_OUT_L` | Left output |

## MCU pin assignments

Not applicable: the design has no MCU or module. J1 carries a shared driver-input net; it is not assigned a GPIO in the schematic.

| Module pin | GPIO | Net | Function |
|---|---|---|---|
| — | — | — | No MCU or module is present in this design. |

## Peripheral details

- **Relay driver IC1 (ULN2003LVDR):** pin 1 / IN1, pin 2 / IN2, pin 3 / IN3, pin 4 / IN4, and pin 5 / IN5 all connect to `Net-(D2-A)`. Pin 6 / IN6 and pin 7 / IN7 are unconnected. Pin 8 / GND is `GND2`; pin 9 / COM is `+5V`. Pin 10 / OUT7 and pin 11 / OUT6 are unconnected. Pins 12 / OUT5, 13 / OUT4, 14 / OUT3, 15 / OUT2, and 16 / OUT1 all connect to `switch power`.
- **Relays:** K1, K2, and K3 are EC2-5SNU DPDT relays. Each has pin 1 on `+5V` and pin 12 on `switch power`. Their connected signal pins are:

  | Ref | Pin-to-net mapping from schematic |
  |---|---|
  | K1 | 3 `inp_1_bal_L_-`; 4 `out_L_-`; 5 `inp_2_bal_L_-`; 8 `inp_2_bal_L_+`; 9 `out_L_+`; 10 `inp_1_bal_L_+` |
  | K2 | 3 `inp_1_bal_R_-`; 4 `out_R_-`; 5 `inp_2_bal_R_-`; 8 `inp_2_bal_R_+`; 9 `out_R_+`; 10 `inp_1_bal_R_+` |
  | K3 | 3 `SE_1_L`; 4 `SE_OUT_L`; 5 `SE_2_L`; 8 `SE_2_R`; 9 `SE_OUT_R`; 10 `SE_1_R` |

- **Power and indicators:** J3 supplies `+5V` and `GND2`; C1 is 0.1 between those rails. D1/R1 and D2/R2 are shown in the power/signal-control circuitry; confirm LED polarity and the intended indication before assembly.
- **Ground reference:** R3 is 10R between `sig ground` and `GND2`. J2 connects to `Earth`.
- **Other circuitry:** No expander or level shifter is shown.
- **Schematic/PCB net-name differences:** PCB pad connectivity was checked against the schematic. The PCB export normalizes local labels (for example `sig ground` appears as `/SIG?GROUND`), and the schematic net `Net-(D2-A)` appears on the PCB as `NET-(IC1-IN1)`. The pin connectivity matches; schematic labels are used throughout this document.

## Full BOM

Transcribed from the supplied [production BOM CSV](<BOM.csv>). Its `LCSC Part #` column is blank for all rows. The C1 value field contains `C0603C104K4RAC3190`.

| Ref | Qty | Value / part | Footprint | Supplier PN (LCSC) |
|---|---:|---|---|---|
| C1 | 1 | C0603C104K4RAC3190 | 0603 | — |
| D1, D2 | 2 | LS_R976 | LEDC2012X40N | — |
| H1, H2, H3, H4 | 4 | MountingHole | MountingHole_2.5mm | — |
| IC1 | 1 | ULN2003LVDR | SOIC127P600X175-16N | — |
| J1, J10, J12, J13, J14, J15, J16, J17, J18, J19, J3 | 11 | 1725669 | 1725669 | — |
| J2 | 1 | Conn_01x01_Socket | PinHeader_1x01_P1.00mm_Vertical | — |
| K1, K2, K3 | 3 | EC2-5SNU | Relay_DPDT_Kemet_EC2 | — |
| R1, R2 | 2 | 1k | 0603 | — |
| R3 | 1 | 10R | 0603 | — |

## Notes

- The board outline is 65.0 x 50.5 mm, according to KiCad board statistics.
- H1-H4 are mounting-hole footprints; the PCB statistics report four 2.5 mm non-plated holes. Verify screw and enclosure clearances.
- Original project filenames are `Relay OUT Signal switch board.kicad_pro`, `Relay OUT Signal switch board.kicad_sch`, and `Relay OUT Signal switch board.kicad_pcb`; the supplied BOM is `production/bom.csv`. The schematic title block has no title; its date is 2024-12-06 and revision is v1.0.
- Verify the supply current rating, the role and polarity of D1/D2, connector orientation against the physical silkscreen, and the `Earth`/signal-ground/chassis-grounding scheme before assembly.
- The supplied production BOM lists IC1's footprint as `SOIC127P600X175-16N`; the PCB and schematic BOM export use `SOIC127P780X200-16N`. Verify the intended IC1 footprint before manufacturing.
- The schematic says all lines are switched together, unless testing; confirm the intended relay control behavior before connecting equipment.

## Review for next version

Issues identified from the current design documentation and files:

1. **IC1 footprint mismatch.** The production BOM lists `SOIC127P600X175-16N`, while the schematic and PCB use `SOIC127P780X200-16N`. Make them consistent before ordering.
2. **Control-input design is unclear.** IC1 inputs 1–5 are tied to one net (`Net-(D2-A)`), with D2 and its resistor on that net. The role and polarity of D1/D2 are unclear, and the control drive level and current are not documented. Clarify the control scheme, including the "unless testing" note.
3. **Grounding scheme is ambiguous.** `GND2`, `sig ground` (joined by 10 Ω R3) and `Earth` (J2) are separate. Document the intended grounding and consider a chassis/earth option.
4. **Power ratings missing.** No supply current rating, no input protection, and only a single 0.1 µF capacitor (C1) for three relays and the driver; add bulk decoupling and a fuse or reverse-polarity protection.
5. **Relay state not visible.** There is no indicator showing which input is selected; the LEDs are not tied to the relay output.
6. **No signal-path protection.** No series resistors, pull-downs or termination on the switched audio lines, so inputs float when disconnected.
7. **Naming and housekeeping.** The C1 value field holds a manufacturer part number (`C0603C104K4RAC3190`), the BOM mounting-hole footprint (`MountingHole_2.5mm`) differs from other boards, the schematic title is blank, the LCSC column is empty, and net names differ between schematic and PCB (`sig ground` vs `/SIG?GROUND`).
