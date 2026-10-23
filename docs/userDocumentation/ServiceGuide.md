# Riverbank Streamer DAC

## Service and Build Guide

**Author:** Anthony Roy

<figure>
<img src="../user-manual-media/media/service-guide/streamer-dac-front-render.png" style="width:6.26772in;height:4.5748in" />
<figcaption><p>Riverbank Streamer DAC</p></figcaption>
</figure>

## Safety Information

> **Warning — hazardous mains voltage:** This project uses a toroidal transformer connected to 110 V or 230 V AC mains. Contact with exposed mains connections can cause fatal electric shock or severe injury.

- Insulate all mains connections—including the IEC inlet, fuse holder, power switch, and transformer primary wiring—with suitable heat-shrink tubing or equivalent insulation.
- Disconnect the power cord from the wall outlet before opening the chassis or working on the equipment.
- Install a slow-blow fuse of the correct rating in the live AC line. Select the fuse for the transformer and the local mains supply; transformer inrush current can be high.
- Observe DC polarity. Reversed connections can damage microcontrollers, DACs, and other components. Use ESD precautions when handling exposed semiconductor devices.

This guide is provided for informational and educational purposes. Construction and use are undertaken at the builder's own risk. Mains-powered equipment should be assembled, inspected, and tested only by people qualified to work safely with hazardous voltages.

**Reference:** [Transformer wiring](https://electro-dan.co.uk/electronics/wiringtrans.aspx)

---

## Contents

- [Project Overview](#project-overview)
- [Circuit Board Reference](service-guide/CircuitBoardReference.md)
- [Mechanical Construction](service-guide/MechanicalConstruction.md)
- [System Wiring and Commissioning](service-guide/SystemWiring.md)
- [Parts Lists](#parts-lists)
- [Revision History](#revision-history)

Use the focused guides for board-level information, construction, wiring, and subsystem-specific details. This index provides the project overview and links to the system diagrams. Electrical details and parts should be verified against the current schematics and manufacturer documentation before construction or repair.

## Project Overview

The Riverbank Streamer DAC combines a Raspberry Pi running moOde Audio, the TinyControlBoard
controller, digital audio modules, and analogue output stages in a custom enclosure. For
the canonical system boundaries, control flow, and architecture diagrams, see
[01 — System Architecture](../01-system-architecture.md).

This build guide focuses on construction and service. The audio assembly includes a
FIFOPi Q7 reclocking stage, selectable ESS DAC and ProtoDAC paths, and independent linear
supplies for selected digital, analogue, control, and clock domains. UcPure
ultra-capacitor modules provide local energy storage and conditioning in selected circuits.

The documented digital audio sources are:

- Raspberry Pi network streaming via I2S
- Optical (TOSLINK) S/PDIF input
- Coaxial S/PDIF input
- USB input

The audio source and output topology is shown in the architecture document. For board
details, consult the focused [DAC and audio subsystem guide](service-guide/DacSubsystem.md).

## Parts Lists

- [Main assembly bill of materials](service-guide/MechanicalConstruction.md#main-assembly-bill-of-materials)
- [Power supply bill of materials](service-guide/PowerSupplySubsystem.md#bill-of-materials)

## Revision History

| Revision | Date | Changes         |
| -------- | ---- | --------------- |
| 1.0      | TBD  | Initial release |
