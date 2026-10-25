# Single_ended_to_Diff

![3D board render](board-render.png)

## Overview

| Item | Details |
|---|---|
| MCU / main IC | No MCU. Two 1646S08-U devices (IC1, IC2) form the differential output stages. |
| Supply | Schematic block specifies +5 V (VCC) and -5 V (VEE); J4 also carries GND2. |
| Programming / debug | None shown. |
| Source files | `se_to_diff board.kicad_pro`, `se_to_diff board.kicad_sch`, `se_to_diff board.kicad_pcb`, `bom.csv`. |

## Schematic

![Schematic, sheet 1](schematic/schematic-sheet-01.png)

One schematic sheet was found. Open the exported [PDF](schematic/se_to_diff_board.pdf) or [SVG](<schematic/se_to_diff board.svg>).

## Functional blocks

| Block (schematic title) | Parts | Description |
|---|---|---|
| Power Supply +5v Vcc, -5V Vee | J4, D1-D2, R1-R2, C1-C2, C5, C7, R5, c8 | Brings in VCC, GND2, and VEE; includes rail indicator components, bulk capacitors, and the GND/GND2 link. |
| Single ended Input | J1, C3-C4, R3-R4 | Two single-ended signal inputs, labeled `in L` and `in R`, with associated passive components. |
| Differential Stage | IC1-IC2, c9-c16, C13 | Two 1646S08-U stages with local capacitors. |
| Output Stages | 22R1-22R3, R6, FB1-FB4, J2-J3 | Series resistors and ferrite beads feed the left and right differential output connectors. |
| Mounting Holes | H1-H4 | Four mounting-hole footprints. |

## Connector pinouts

Pin numbers and net assignments below follow the schematic and were checked against the PCB pad nets. Connector orientation is not implied by this table.

### J1 — single-ended input

| Pin | Schematic signal | PCB net |
|---:|---|---|
| 1 | Left input | `/in L` |
| 2 | Signal ground | `GND` |
| 3 | Right input | `/in R` |

### J2 — left output

| Pin | Schematic signal | PCB net |
|---:|---|---|
| 1 | Left output +, after FB2 | `Net-(FB2-Pad2)` |
| 2 | Ground | `GND` |
| 3 | Left output -, after FB1 | `Net-(FB1-Pad2)` |

### J3 — right output

| Pin | Schematic signal | PCB net |
|---:|---|---|
| 1 | Right output +, after FB4 | `Net-(FB4-Pad2)` |
| 2 | Ground | `GND` |
| 3 | Right output -, after FB3 | `Net-(FB3-Pad2)` |

### J4 — power input

| Pin | Schematic signal | PCB net |
|---:|---|---|
| 1 | +5 V rail | `VCC` |
| 2 | Supply return | `GND2` |
| 3 | -5 V rail | `VEE` |

The output connector wires have no explicit labels after their ferrite beads; their signal functions above are traced from the schematic wiring. The PCB uses the corresponding auto-generated `Net-(FBx-Pad2)` names.

## MCU pin assignments

Not applicable: no MCU or module appears in the schematic or PCB.

## Peripheral details

- **Differential drivers:** IC1 and IC2 are marked `1646S08-U`. The schematic symbol assigns pins 1-8 as OUT-, SNS-, GND, IN, VEE, VCC, SNS+, and OUT+, respectively. IC1 receives `in L`; IC2 receives `in R`.
- **Output paths:** Each driver output passes through 22-ohm series resistance and one of FB1-FB4 before reaching J2/J3. The connector-side nets are auto-named in the design files.
- **Input network:** J1 carries left and right single-ended inputs with C3/C4 (220 pF) and R3/R4 (47 kOhm) shown around the input lines.
- **Power and ground:** J4 supplies VCC, GND2, and VEE. The schematic title specifies +5 V and -5 V rails. R5 (10 ohm) and c8 (100 nF) connect GND2 to signal ground GND; C1/C2/C5/C7 provide rail capacitors, with additional driver-local capacitors.
- **Indicators:** D1/D2 and R1/R2 are in the supply section. Their exact LED color/polarity is not documented here beyond the schematic symbol and connectivity.

## BOM

Transcribed from the supplied `bom.csv`. Supplier part numbers are blank in that file; `—` means not provided.

| Reference(s) in CSV | Qty | Value / part | Footprint in CSV | Supplier PN (LCSC) |
|---|---:|---|---|---|
| C1, C2, C5, C7 | 4 | 47u | CAPAE660X800N | — |
| C10, C11, C13, C9 | 4 | 10u | CAPC3216X229N | — |
| C12, C14, C15, C16, C8 | 5 | 100n | CAPC2012X94N | — |
| C3, C4 | 2 | 220p | 0805 | — |
| D1, D2 | 2 | LS_R976 | LEDC2012X40N | — |
| FB1, FB2, FB3, FB4 | 4 | NFZ2MSD101SZ10L | BEADC2016X120N | — |
| H1, H2, H3, H4 | 4 | MountingHole | MountingHole_2.5mm | — |
| IC1, IC2 | 2 | 1646S08-U | SOIC127P602X173-8N | — |
| J1, J2, J3, J4 | 4 | 1725669 | 1725669 | — |
| R1_2, R2, R3_2 | 3 | 22 | 0603 | — |
| R1, R2_2 | 2 | 3k | ERJH3 | — |
| R3, R4 | 2 | 47k | ERJH3 | — |
| R5 | 1 | 10R | 0603 | — |
| R6 | 1 | 22R | 0603 | — |

## Notes

- The Edge.Cuts outline is a 65 mm x 56.5 mm rectangle (PCB coordinates 100,50 to 165,106.5).
- H1-H4 use the `MountingHole_2.5mm` footprint. The exact drilled-hole and clearance dimensions have not been independently verified.
- Source project filenames contain spaces: `se_to_diff board.kicad_pro`, `se_to_diff board.kicad_sch`, and `se_to_diff board.kicad_pcb`.
- The supplied BOM's 22-ohm designators (`R1_2`, `R2`, `R3_2`) do not match the schematic designators (`22R1`, `22R2`, `22R3`); verify this BOM reference mapping. The separate `R6` 22R entry is consistent.
- The BOM lists `R2_2` for one 3k resistor, while the schematic uses `R1` and `R2`; verify the suffixed reference.
- The checked pad-to-net mappings for J1-J4 and IC1-IC2 agree between the schematic and PCB. The J2/J3 connector-side names are autogenerated PCB net names rather than explicit schematic labels.
- No explicit programming/debug interface or MCU is shown.
