> **Superseded in part.** See the full [Six-Channel Relay Board engineering and service document](SixChannelRelayBoard/SixChannelRelayBoard-EngineeringAndService.md). Corrections: connectors are 2.54 mm pin headers (not screw terminals); the EE2-5NU relay is 2 Form C with one pair brought out (normally open, inferred); the schematic exists in KiCad, so it is no longer read from a screenshot.

# 6-Channel Relay Board

[Back to the Circuit Board Reference](../CircuitBoardReference.md) · [Service Guide](../../ServiceGuide.md)

A six-relay switching board. Logic-level control inputs drive a ULN2003LV transistor array, which energises six 5 V relays. Each relay brings out one switched contact pair on a screw terminal.

## What It Does

**Signal path:** J2 (control inputs) → IC1 ULN2003LVDR → relay coils K1–K6 → contact pairs on J4 and J5

An input going high turns on the matching driver output, which pulls the relay coil to GND and closes the relay.

## Connectors

| Ref | Use | Pins |
|---|---|---|
| J1 | Power input | 1 = GND, 2 and 3 = supply (+5 V, see Notes) |
| J2 | Control inputs | 1 to 6 = Relay 1 In to Relay 6 In |
| J4 | Relay contacts, K1 to K3 | 1-2 = K1, 3-4 = K2, 5-6 = K3 |
| J5 | Relay contacts, K4 to K6 | 1-2 = K4, 3-4 = K5, 5-6 = K6 |

All connectors are screw terminals.

## Input-to-Relay Mapping

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

## Components

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

## Quick Checks

| Check | Expected |
|---|---|
| J1 pin 2 or 3 to pin 1 | Supply voltage (+5 V) |
| D1 | Lit when powered |
| Relay coil (pin 1 to pin 8) with input high | About supply voltage, relay clicks |
| Relay coil with input low | 0 V across the coil |

## Quick Troubleshooting

| Symptom | Check |
|---|---|
| D1 off | Supply on J1, polarity, then R1 and D1 |
| No relay operates | Supply on the relay rail, IC1 pin 8 (GND) and pin 9 (COM) |
| One relay does not operate | Input signal on J2, then IC1 input and output, then the coil |
| Wrong relay switches | Input numbering is reversed (see mapping table) |
| Relay clicks but no contact change | Wiring on J4 or J5, then the relay contacts |
| Relay stuck on | Input pin held high, shorted IC1 output, or damaged relay |
| Intermittent resets or noise | C1 and the supply rail |

## Notes

- The supply voltage is not labelled in the schematic. The EE2-5NU coils are 5 V, so it should be +5 V. Confirm before connecting.
- The reversed numbering between J2 and the relays is easy to miss. Check which relay you are testing.
- Whether the contact on pin 5 is normally open or normally closed depends on the relay datasheet. Confirm before wiring the load.
- Pin and net details were read from a screenshot. Confirm against the KiCad schematic before using them for repairs.


