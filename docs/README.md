# TinyControlBoard

## Project Overview

TinyControlBoard is the front-panel control and power-sequencing system for a Raspberry Pi based
[moOde Audio](https://moodeaudio.org/) music streamer (the **Streamer DAC**). It brings together:

- **Custom hardware** – an enclosure, custom circuit boards and an external linear power supply.
- **ESP32 firmware** – reads the front-panel buttons and controls the display, relays and power sequencing.
- **Raspberry Pi software** – runs moOde and the scripts that talk to the control board.
- **DanStreamer Android app** – a phone remote control for playback, library browsing, display settings and power.

## User Documentation

Start here if you want to use the Streamer DAC or the phone app.

| Document                                                            | Description                                                                                                                                 |
| ------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| [Streamer DAC User Manual](userDocumentation/Dac-user-manual.md)     | Setting up and operating the Streamer DAC and its external power supply: quick start, everyday use, troubleshooting and safety information. |
| [Android App User Manual](userDocumentation/AndroidAppUserManual.md) | Using the DanStreamer phone app to control playback, find and queue music, adjust the display and control the power state.                  |

## Project Documentation

Technical documentation for building and developing the project, split into hardware and software.

### Hardware Documentation

#### Enclosure and Case Build

Drawings, dimensions and assembly views for building the cases. Read them in this order:

1. [Dac-Case.md](HardwareDocumentation/Enclosure/Dac-Case.md) – panel drawings and dimensions for the main DAC enclosure, the inner case and the power supply enclosure.
2. [Component-Assemblies.md](HardwareDocumentation/Enclosure/Component-Assemblies.md) – how the internal components are fitted to the panels and cases.

#### Custom PCBs

See the [Custom Boards index](<Custom boards/README.md>) for a list of every custom board with a short description.

See the [KiCad Guide](HardwareDocumentation/KiCad-Guide.md) for the project workflow and
current repository status. 

#### Wiring Diagrams and Bill of Materials

See the [Power Distribution Diagram](WiringDiagrams/PowerDistribution.md) and the
[Bill of Materials](bom/BOM.md) for the complete parts list.

For the GPIO, relay and wiring details used by the software, see
[06-hardware-interface.md](DeveloperGuides/06-hardware-interface.md).

### Software Documentation

The [system architecture document](01-system-architecture.md) is the canonical source for system
boundaries, component responsibilities, communication flows, deployment, quality requirements and
the control/audio diagrams. Read it first. The developer guides below are the authoritative
references for their respective areas.

| Guide                                                                         | Description                                                                                             |
| ----------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| [01-system-architecture.md](01-system-architecture.md)                         | Overall system architecture and diagrams. **Start here.**                                         |
| [01-Rpi4_SETUP.md](DeveloperGuides/01-Rpi4_SETUP.md)                           | Setting up the Raspberry Pi 4.                                                                          |
| [02-android-developer-guide.md](DeveloperGuides/02-android-developer-guide.md) | Working on the Android app.                                                                             |
| [03-esp32-developer-guide.md](DeveloperGuides/03-esp32-developer-guide.md)     | Working on the ESP32 firmware.                                                                          |
| [04-rpi-developer-guide.md](DeveloperGuides/04-rpi-developer-guide.md)         | Working on the Raspberry Pi side.                                                                       |
| [05-communication-protocol.md](DeveloperGuides/05-communication-protocol.md)   | The wire protocol. Read before touching `uartProtocol.hpp`, `protocol.py` or `BleProtocol.kt`.    |
| [06-hardware-interface.md](DeveloperGuides/06-hardware-interface.md)           | Wiring, GPIO and relays.                                                                                |
| [07-development-environment.md](DeveloperGuides/07-development-environment.md) | Setting up a development machine.                                                                       |
| [08-quality-check.md](DeveloperGuides/08-quality-check.md)                     | Known gaps and cross-component consistency checks.                                                      |
| [09-adding-button-actions.md](DeveloperGuides/09-adding-button-actions.md)     | Adding a new button or action (a matching `/add-button-action` prompt lives in `.github/prompts/`). |
