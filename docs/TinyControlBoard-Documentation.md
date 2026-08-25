# TinyControlBoard Documentation

This document is the consolidated entry point for the current TinyControlBoard
firmware, Raspberry Pi companion, and Android remote. Detailed references are
kept in focused documents so they can be updated alongside the subsystem they
describe.

## Current System Summary

TinyControlBoard is an ESP32-S3 control surface for a Raspberry Pi music player.
The firmware initializes board hardware, converts MCP23017 button and rotary
events into `IAction` objects, and executes them through `ActionProcessor`.
Actions either change local hardware or send framed UART commands to the Pi.

After successful initialization, `app_main()` starts the BLE GATT server. BLE
commands use the same `ActionProcessor::injectCommand()` path as remote command
injection; library browse commands use a dedicated UART callback. The Pi sends
heartbeat, now-playing, track-progress, and library-entry messages back to the
ESP32.

Power-on is staged through the screen, DAC, output-stage, and Raspberry Pi
relays. `BootDiagnosticLeds` lights eight SPI LEDs during the sequence, turns
off each stage pair on success, and flashes a failed pair at about 3 Hz. Firmware
initialization failures flash all eight diagnostic LEDs while `app_main()` retries
initialization every second.

## Reference Documents

- [Project guide](./project-guide.md) — scope, current layout, hardware roles, and build overview
- [Architecture](./architecture.md) — runtime flow, ownership, concurrency, and recovery
- [Wiring reference](./wiring-reference.md) — ESP32-S3, Pi, relay, indicator, and connector mappings
- [Protocol reference](./protocol-reference.md) — variable-length UART framing, messages, and command IDs
- [Power sequencing](./power-sequencing.md) — power states, relay order, timing, and heartbeat coordination
- [Button and command map](./button-command-map.md) — physical input, LED state, and command behavior
- [Add-button guide](./add-button-how-to.md) — current registration and action-extension workflow
- [Raspberry Pi setup](./raspberry-pi-setup.md) — Pi services, UART5, GPIO handshake, and companion scripts
- [Android TinyRemote guide](./android-tinyremote-user-manual.md) — end-user BLE remote operation
- [Android project README](../android/TinyRemote/README.md) — Android build and deployment

## Plans And Task Tracking

- [Project structure plan](./project-structure-plan.md) — migration record; current layout is in the architecture reference
- [WiFi transport plan](./wifi-transport-plan.md) — design notes and implementation status for WiFi transport
- [Library browse plan](./library-browse-plan.md) — design history for the implemented library and playlist features
- [TASKS.md](../TASKS.md) — active validation and hardware tasks

## Current Validation

- Host logic tests: `41/41` passing in `host_tests/build/Debug/tiny_control_board_host_tests.exe`.
- Firmware build: use the VS Code `PlatformIO Build` task.
- Device tests: under `test/`; require a connected ESP32-S3.
- Raspberry Pi scripts: `scripts/rpi/home/antho/uart5_listener.py` is the canonical listener entry point; sibling modules are deployed with it.

## Canonical Source Areas

| Area | Responsibility |
|---|---|
| `src/` | ESP-IDF entry point and component registration |
| `lib/app/` | orchestration, action processing, and command implementations |
| `lib/board/` | board pins, timing, identity, and debug configuration |
| `lib/hal/` | hardware drivers and UART transport collaborators |
| `lib/input/actions/` | reusable action-source templates |
| `lib/indicators/` | power, status, brightness, and boot diagnostics |
| `lib/power/` | relay control and power-state sequencing |
| `lib/protocol/` | UART wire format and command catalog |
| `lib/ble/` | BLE GATT server and remote-control notifications |
| `scripts/rpi/` | Raspberry Pi listener, heartbeat sender, and deployment assets |
| `android/TinyRemote/` | Android BLE remote application |
| `host_tests/` | hardware-independent C++ logic tests |
| `test/` | PlatformIO tests for the ESP32-S3 |

The focused documents above are authoritative when this index and an older
planning note disagree.
