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

## Setup Checklist

The project has several required setup steps that are easy to miss if you start
from the architectural docs alone.

### Firmware setup

- PlatformIO is the verified build path for the ESP32-S3 firmware.
- The project is configured for `espressif32 @ ~6.5.0` and the
  `esp32-s3-devkitc-1-16mb` board in [platformio.ini](../platformio.ini).
- Use the VS Code `PlatformIO Build` task for firmware builds and the matching
  PlatformIO upload flow for flashing a connected board.

### Raspberry Pi setup

- The Pi must run a Moode-based audio stack and have `dtoverlay=uart5` enabled in
  `/boot/firmware/config.txt`.
- The companion Python scripts are expected to be deployed together in
  `/home/antho`, including:
  - `uart5_listener.py`
  - `heartbeat_sender.py`
  - `protocol.py`
  - `panel_control.py`
  - `playback_commands.py`
  - `library_browser.py`
  - `playlist_manager.py`
  - `mpd_client.py`
  - `command_ids.py`
  - `uart_writer_client.py`
- The listener uses `/dev/ttyAMA5`, GPIO 23 for the ESP32 data-ready input, and
  the heartbeat sender uses GPIO 24 for the data-ready pulse.
- The Pi must not have a conflicting serial console enabled on the UART5 path.
- Required Python packages include `pyserial`, `RPi.GPIO`, and `requests`.
- The heartbeat logic also depends on the `mpc` CLI being installed and available
  for now-playing/progress reporting.
- The intended systemd services are `uart_listener.service` and
  `heartbeat.service`, both started together for the full control loop.

### Android app setup

- Android Studio with the bundled JBR / JDK 17 is required.
- The Android SDK must include API 34 (`compileSdk` / `targetSdk`).
- The project is opened from `android/TinyRemote` and may require a generated
  `local.properties` file pointing at the installed Android SDK.
- USB debugging must be enabled on the Android device for local deployment.
- On first launch, Bluetooth and location permissions are required depending on
  the Android version.

### Related setup references

- [docs/project-guide.md](./project-guide.md)
- [docs/raspberry-pi-setup.md](./raspberry-pi-setup.md)
- [scripts/rpi/RPI4_SETUP.md](../scripts/rpi/RPI4_SETUP.md)
- [android/TinyRemote/README.md](../android/TinyRemote/README.md)

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
