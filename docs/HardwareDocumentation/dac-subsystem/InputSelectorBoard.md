# Input Selector Board

[DAC and audio subsystem](../DacSubsystem.md) · [Circuit Board Reference](../CircuitBoardReference.md) · [Service Guide](../../ServiceGuide.md)

A two-input analogue source selector. Three relays switch between **Input 1** and **Input 2** for stereo balanced and stereo single-ended signals. All relays switch together from one control input.

## What It Does

- **K1** switches the left balanced signal, **K2** the right balanced signal, and **K3** the stereo single-ended signal.
- Each relay is a two-pole changeover. With the relay **off**, **Input 1** is routed to the output. With the relay **on**, **Input 2** is routed to the output.
- A ULN2003LV driver (IC1) energises all three relay coils together when the control input is driven.

**Signal path:** input connectors → K1 / K2 / K3 contacts → output connectors J17, J18, J19

## Connectors

All connectors are 3-pin (1725669) unless noted.

### Signal Inputs

| Ref | Signal | Pin 1 | Pin 2 | Pin 3 |
|---|---|---|---|---|
| J15 | Input 1 balanced left | − | sig ground | + |
| J16 | Input 1 balanced right | − | sig ground | + |
| J13 | Input 2 balanced left | − | sig ground | + |
| J14 | Input 2 balanced right | − | sig ground | + |
| J10 | Input 1 single-ended | L | sig ground | R |
| J12 | Input 2 single-ended | L | sig ground | R |

### Signal Outputs

| Ref | Signal | Pin 1 | Pin 2 | Pin 3 |
|---|---|---|---|---|
| J18 | Balanced left out | + | sig ground | − |
| J17 | Balanced right out | + | sig ground | − |
| J19 | Single-ended out | R | sig ground | L |

**The output pin order is reversed compared with the inputs.** On the balanced outputs, pin 1 is + and pin 3 is −. On the single-ended output, pin 1 is R and pin 3 is L.

### Power and Control

| Ref | Use | Pins |
|---|---|---|
| J3 | 5 V DC power | 1 = GND, 2 and 3 = +5 V |
| J1 | Switch control input | 1 = GND, 2 and 3 = control (tied together) |
| J2 | Earth | Single socket to chassis earth |

## Relays

| Ref | Part | Switches | Off (Input 1) | On (Input 2) |
|---|---|---|---|---|
| K1 | EC2-5SNU | Balanced left | Pins 3 and 10 | Pins 5 and 8 |
| K2 | EC2-5SNU | Balanced right | Pins 3 and 10 | Pins 5 and 8 |
| K3 | EC2-5SNU | Single-ended L and R | Pins 3 and 10 | Pins 5 and 8 |

- Common (output) contacts are pin 4 (− or L) and pin 9 (+ or R).
- The coil is between +5 V (pin 1) and `switch power` (pin 12), which IC1 pulls to ground.

## Control Circuit

- **J1:** driving the control input high turns all three relays on.
- **IC1 (ULN2003LVDR):** IN1 to IN5 are tied together, and OUT1 to OUT5 are tied together, which gives one high-current sink for all three coils. IN6, IN7, OUT6 and OUT7 are unused. COM goes to +5 V.
- **D2 and R2 (1 kΩ):** indicator LED, lit when the control input is high.
- A schematic note says all lines are switched together unless testing.

## Power

- **J3** supplies +5 V. **D1 with R1 (1 kΩ)** is the power indicator.
- **C1 (100 nF)** decouples the 5 V rail.
- **R3 (10 Ω)** links the signal ground (`sig ground`) to the power ground (`GND2`). They are not connected directly.

## Quick Checks

| Check | Expected |
|---|---|
| J3 pin 2 or 3 to pin 1 | +5 V |
| D1 | Lit when powered |
| J1 control low | D2 off, relays off, Input 1 selected |
| J1 control high | D2 lit, relays click, Input 2 selected |
| Relay coil voltage (pin 1 to pin 12) | About 5 V when control is high, 0 V when low |

## Quick Troubleshooting

| Symptom | Check |
|---|---|
| No power LED | J3 supply and polarity, then R1 and D1 |
| No relay operates | +5 V on the relay rail, IC1 GND (pin 8) and COM (pin 9), control signal at J1 |
| D2 lit but relays do not switch | IC1 outputs, `switch power` net, relay coils |
| D2 never lights | Control signal on J1, R2 and D2 |
| No signal at output | Selected input connected, relay contacts, output wiring |
| Left or right channel reversed or inverted | Output pin order is reversed (see connectors) |
| Hum or noise | R3 ground link, input grounds, earth connection on J2 |

## Notes

- Relay symbols show S+ and R+ coil markings. The part (EC2-5SNU) should be a single-coil non-latching relay; confirm with the datasheet if a relay does not return to Input 1 when control is released.
- The control input has no stated voltage range in the schematic. Check the ULN2003LV datasheet for the input threshold before connecting a controller.
- Pin and net details were read from a screenshot. Confirm against the KiCad schematic before using them for repairs.
