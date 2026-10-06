# DAC and Audio Subsystem

[Back to the Service Guide](ServiceGuide.md) · [System wiring and commissioning](SystemWiring.md)

This guide groups the existing DAC and analogue-audio material. Verify board revisions, signal polarity, connector pinouts, and component specifications against the current schematics before use.

## Single-Ended to Balanced Line Driver

<img src="../user-manual-media/media/service-guide/single-ended-to-balanced-output-schematic.png" style="width:6.26806in;height:4.32917in" />

<img src="../user-manual-media/media/service-guide/single-ended-to-balanced-output-board.png" style="width:6.26806in;height:5.43958in" />

Stereo converter board: unbalanced stereo input in, balanced stereo output out, powered from a dual ±5 V supply.

### What It Does

Each channel passes through one THAT 1646 balanced line driver (IC1 left, IC2 right; the schematic label reads `1646S08-U`). The driver converts the single-ended input into a differential output, which leaves through a 22 Ω resistor and a ferrite bead on each leg.

**Signal path:** J1 (input) → input network → IC1 / IC2 → 22 Ω + ferrite bead per leg → J2 (left out) / J3 (right out)

### Connectors

All connectors are 3-pin (1725669).

| Ref | Use | Pin 1 | Pin 2 | Pin 3 |
|---|---|---|---|---|
| J1 | Input | In L | GND | In R |
| J2 | Left output | Sout L + | GND | Sout L − |
| J3 | Right output | Sout R + | GND | Sout R − |
| J4 | Power input | VCC (+5 V) | GND2 (0 V) | VEE (−5 V) |

### Power Supply

- J4 takes a **dual supply: +5 V (VCC) and −5 V (VEE), with 0 V on GND2**.
- Each rail has 2 x 47 µF bulk capacitors: C7 and C1 on VCC, C2 and C5 on VEE.
- **D1** (with R1, 3 kΩ) and **D2** (with R2, 3 kΩ) are indicator LEDs on the +5 V and −5 V rails *(verify)*.
- **GND2** is the supply ground. It joins the signal **GND** only through **R5 (10 Ω) in parallel with C8 (100 nF)**.
- Each driver has 100 nF decoupling on its supply pins: C12 and C15 on IC1, C14 and C16 on IC2, all returned to GND2.

### Input Stage

For each channel, a **47 kΩ resistor in parallel with 220 pF** goes from the input to GND (R3 and C3 for left, R4 and C4 for right). This sets the input impedance and filters RF. The signal then goes directly to pin 4 (IN) of the driver.

### Driver Stage (IC1 Left, IC2 Right)

| Pin | Name | Connection |
|---|---|---|
| 1 | OUT− | `out −` net |
| 2 | SNS− | Via 10 µF to `out −` (C9 left, C10 right) |
| 3 | GND | GND (signal ground) |
| 4 | IN | Input signal |
| 5 | VEE | −5 V |
| 6 | VCC | +5 V |
| 7 | SNS+ | Via 10 µF to `out +` (C11 left, C13 right) |
| 8 | OUT+ | `out +` net |

### Output Stage

Each output leg has a **22 Ω series resistor** followed by a **ferrite bead**:

| Channel | Resistors | Ferrite beads |
|---|---|---|
| Left | R6, 22R1 | FB1 (−), FB2 (+) |
| Right | 22R2, 22R3 | FB3 (−), FB4 (+) |

### Quick Checks

| Check | Expected |
|---|---|
| J4 pin 1 to pin 2 | +5 V |
| J4 pin 3 to pin 2 | −5 V |
| IC pin 6 to GND2 | +5 V |
| IC pin 5 to GND2 | −5 V |
| D1 and D2 | Both lit |

### Quick Troubleshooting

| Symptom | Check |
|---|---|
| No output on either channel | J4 supply, D1 and D2, then VCC and VEE at the driver pins |
| One channel dead | Input wiring on J1, then pin 4 signal, supply pins and output on the affected IC |
| One output leg dead | The 22 Ω resistor and ferrite bead on that leg, then the connector |
| Hum or noise | R5 / C8 ground link, GND and GND2 connections, input cable |
| Distortion or low level | Output loading, supply voltage under load, sense capacitors (C9, C11 left; C10, C13 right) |
| One rail missing | Supply wiring, the bulk capacitors on that rail, and the supply itself |

### Notes

- The two grounds are intentional: **GND** is the signal ground (input and driver pin 3) and **GND2** is the supply return. Do not link them anywhere except through R5 and C8.
- The driver part number is read from the schematic label. Confirm against the BOM before ordering replacements.


## Differential-to-Single-Ended Output Board

<img src="../user-manual-media/media/service-guide/differential-to-single-ended-output-schematic.png" style="width:6.26806in;height:4.35694in" />

<img src="../user-manual-media/media/service-guide/differential-to-single-ended-output-board.png" style="width:6.26806in;height:5.44444in" />

## ProtoDAC Board

<img src="../user-manual-media/media/service-guide/protodac-schematic.png" style="width:6.26806in;height:4.31319in" />

<img src="../user-manual-media/media/service-guide/protodac-pcb-render.png" style="width:6.26806in;height:8.13611in" />

## DAC Output Relay Switch

<img src="../user-manual-media/media/service-guide/relay-dac-output-switch-schematic.png" style="width:6.26806in;height:4.29444in" />

<img src="../user-manual-media/media/service-guide/relay-dac-output-switch-board.png" style="width:6.26806in;height:4.92222in" />

## Input Selector Board

A two-input analogue source selector. Three relays switch between **Input 1** and **Input 2** for stereo balanced and stereo single-ended signals. All relays switch together from one control input.

### What It Does

- **K1** switches the left balanced signal, **K2** the right balanced signal, and **K3** the stereo single-ended signal.
- Each relay is a two-pole changeover. With the relay **off**, **Input 1** is routed to the output. With the relay **on**, **Input 2** is routed to the output.
- A ULN2003LV driver (IC1) energises all three relay coils together when the control input is driven.

**Signal path:** input connectors → K1 / K2 / K3 contacts → output connectors J17, J18, J19

### Connectors

All connectors are 3-pin (1725669) unless noted.

#### Signal Inputs

| Ref | Signal | Pin 1 | Pin 2 | Pin 3 |
|---|---|---|---|---|
| J15 | Input 1 balanced left | − | sig ground | + |
| J16 | Input 1 balanced right | − | sig ground | + |
| J13 | Input 2 balanced left | − | sig ground | + |
| J14 | Input 2 balanced right | − | sig ground | + |
| J10 | Input 1 single-ended | L | sig ground | R |
| J12 | Input 2 single-ended | L | sig ground | R |

#### Signal Outputs

| Ref | Signal | Pin 1 | Pin 2 | Pin 3 |
|---|---|---|---|---|
| J18 | Balanced left out | + | sig ground | − |
| J17 | Balanced right out | + | sig ground | − |
| J19 | Single-ended out | R | sig ground | L |

**The output pin order is reversed compared with the inputs.** On the balanced outputs, pin 1 is + and pin 3 is −. On the single-ended output, pin 1 is R and pin 3 is L.

#### Power and Control

| Ref | Use | Pins |
|---|---|---|
| J3 | 5 V DC power | 1 = GND, 2 and 3 = +5 V |
| J1 | Switch control input | 1 = GND, 2 and 3 = control (tied together) |
| J2 | Earth | Single socket to chassis earth |

### Relays

| Ref | Part | Switches | Off (Input 1) | On (Input 2) |
|---|---|---|---|---|
| K1 | EC2-5SNU | Balanced left | Pins 3 and 10 | Pins 5 and 8 |
| K2 | EC2-5SNU | Balanced right | Pins 3 and 10 | Pins 5 and 8 |
| K3 | EC2-5SNU | Single-ended L and R | Pins 3 and 10 | Pins 5 and 8 |

- Common (output) contacts are pin 4 (− or L) and pin 9 (+ or R).
- The coil is between +5 V (pin 1) and `switch power` (pin 12), which IC1 pulls to ground.

### Control Circuit

- **J1:** driving the control input high turns all three relays on.
- **IC1 (ULN2003LVDR):** IN1 to IN5 are tied together, and OUT1 to OUT5 are tied together, which gives one high-current sink for all three coils. IN6, IN7, OUT6 and OUT7 are unused. COM goes to +5 V.
- **D2 and R2 (1 kΩ):** indicator LED, lit when the control input is high.
- A schematic note says all lines are switched together unless testing.

### Power

- **J3** supplies +5 V. **D1 with R1 (1 kΩ)** is the power indicator.
- **C1 (100 nF)** decouples the 5 V rail.
- **R3 (10 Ω)** links the signal ground (`sig ground`) to the power ground (`GND2`). They are not connected directly.

### Quick Checks

| Check | Expected |
|---|---|
| J3 pin 2 or 3 to pin 1 | +5 V |
| D1 | Lit when powered |
| J1 control low | D2 off, relays off, Input 1 selected |
| J1 control high | D2 lit, relays click, Input 2 selected |
| Relay coil voltage (pin 1 to pin 12) | About 5 V when control is high, 0 V when low |

### Quick Troubleshooting

| Symptom | Check |
|---|---|
| No power LED | J3 supply and polarity, then R1 and D1 |
| No relay operates | +5 V on the relay rail, IC1 GND (pin 8) and COM (pin 9), control signal at J1 |
| D2 lit but relays do not switch | IC1 outputs, `switch power` net, relay coils |
| D2 never lights | Control signal on J1, R2 and D2 |
| No signal at output | Selected input connected, relay contacts, output wiring |
| Left or right channel reversed or inverted | Output pin order is reversed (see connectors) |
| Hum or noise | R3 ground link, input grounds, earth connection on J2 |

### Notes

- Relay symbols show S+ and R+ coil markings. The part (EC2-5SNU) should be a single-coil non-latching relay; confirm with the datasheet if a relay does not return to Input 1 when control is released.
- The control input has no stated voltage range in the schematic. Check the ULN2003LV datasheet for the input threshold before connecting a controller.
- Pin and net details were read from a screenshot. Confirm against the KiCad schematic before using them for repairs.

## Analogue Audio

*Analogue signal-flow documentation is still in progress.*

### DAC Stage — To Do

### Output Stage — To Do

### Balanced Outputs — To Do

### Single-Ended Outputs — To Do

*To do: document the analogue signal flow.*
