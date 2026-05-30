# TinyControlBoard Project Guide

This is the landing page for the TinyControlBoard documentation set.

The goal is to explain the project in plain language first, then point to the source files that own each part of the behavior. It is intended to grow over time as the hardware, firmware, and Raspberry Pi side evolve.

## Documentation Set

Use this page as the overview, then jump into the focused reference docs as needed.

Core documents:

- [docs/architecture.md](./architecture.md)
- [docs/add-button-how-to.md](./add-button-how-to.md)
- [docs/button-command-map.md](./button-command-map.md)
- [docs/power-sequencing.md](./power-sequencing.md)
- [docs/wiring-reference.md](./wiring-reference.md)
- [docs/protocol-reference.md](./protocol-reference.md)
- [docs/raspberry-pi-setup.md](./raspberry-pi-setup.md)
- [docs/project-structure-plan.md](./project-structure-plan.md)

## 1. What This Project Does

TinyControlBoard is an ESP32-S3-based control surface that:

- reads physical buttons and rotary input,
- controls relays and indicator outputs,
- talks to a Raspberry Pi over UART,
- reacts to Raspberry Pi heartbeat status,
- translates user input into commands such as playback control, display control, DAC control, and power-state changes,
- visually signals boot progress and failure via SPI-driven button LEDs.

In practical terms, the board is the hardware front end and the Raspberry Pi is the system it controls.

## 2. System At A Glance

The main runtime flow is:

1. The ESP32 boots and waits briefly for power to settle.
2. `app_main()` creates a `ControlBoard` instance.
3. `ControlBoard::init()` initializes LEDs, relays, input handling, and serial communication.
4. Button and rotary events are converted into `std::unique_ptr<actions::IAction>` objects via `ActionFactory::createAction()`.
5. `ActionProcessor::process()` executes the action via `IAction::execute(ActionContext&)`.
6. Some actions change local hardware state, and others send UART commands to the Raspberry Pi.
7. The Raspberry Pi sends heartbeat packets back so the ESP32 knows the Pi is still online.

Primary entry point:

- [src/main.cpp](../src/main.cpp)
- [lib/app/ControlBoard.hpp](../lib/app/ControlBoard.hpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)

## 3. Core Runtime Pieces

### 3.1 `main.cpp`

`app_main()` is intentionally small. It delays briefly, creates the control board object, initializes it, and then keeps the firmware alive.

Reference:

- [src/main.cpp](../src/main.cpp)

### 3.2 `ControlBoard`

`ControlBoard` is the top-level orchestrator for the firmware. It wires together the major subsystems and owns the event callbacks for buttons, rotary movement, and received UART messages.

Responsibilities include:

- setting initial LED and brightness state,
- initializing the serial bus singleton,
- starting heartbeat monitoring,
- initializing relays,
- initializing the MCP input handler,
- creating the button-to-action map,
- forwarding action results to the action processor.

References:

- [lib/app/ControlBoard.hpp](../lib/app/ControlBoard.hpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)

### 3.3 `ActionProcessor`

`ActionProcessor` is the decision layer between input events and side effects. It receives `std::unique_ptr<actions::IAction>` objects and dispatches them via `IAction::execute(ActionContext&)`.

Concrete command classes in `lib/app/commands/` carry out the actual work:

- `RelayAction` — local relay changes,
- `UartDispatchAction` — sends UART commands to the Raspberry Pi,
- `PowerTransitionAction` — coordinates Raspberry Pi boot and shutdown,
- `BrightnessAction` — local brightness cycling,
- `DisplayAction` — display on/off,
- `SystemAction` — system-level commands.

References:

- [lib/app/actionProcessor.hpp](../lib/app/actionProcessor.hpp)

### 3.4 Serial Bus

The serial layer manages UART communication with the Raspberry Pi. It also manages the extra GPIO handshake lines used to signal when data is ready on either side.

Main responsibilities:

- configure UART,
- configure GPIO interrupt/output handshake pins,
- send UART packets,
- receive UART packets,
- track heartbeat timing,
- invoke a callback when a complete message is received.

References:

- [lib/transport/uart/serial.hpp](../lib/transport/uart/serial.hpp)
- [lib/transport/uart/serial.cpp](../lib/transport/uart/serial.cpp)

### 3.5 Protocol Layer

The protocol layer defines the wire format used between the ESP32 and the Raspberry Pi.

It owns:

- packet size,
- start byte,
- message layout,
- command IDs,
- app IDs,
- message structure,
- serialization and deserialization entry points.

Reference:

- [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp)

## 4. Input, Outputs, And Hardware Roles

### 4.1 Inputs

User input appears to come primarily from an MCP23017-based I2C input expander and rotary input handling.

Relevant code:

- [lib/hal/buttons/mcpInputHandler.hpp](../lib/hal/buttons/mcpInputHandler.hpp)
- [lib/hal/buttons/mcpInputHandler.cpp](../lib/hal/buttons/mcpInputHandler.cpp)

The button index mapping currently lives in:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)

### 4.2 Relays And Power Control

The board controls multiple relay outputs for major subsystems, including:

- screen power,
- DAC power,
- Raspberry Pi power,
- output stage power,
- prototype DAC enable,
- general-purpose relay outputs.

The current relay pin assignments live in:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)

Related code:

- [lib/hal/relay/relay.hpp](../lib/hal/relay/relay.hpp)
- [lib/power/RelayController.hpp](../lib/power/RelayController.hpp)
- [lib/power/RPIBootManager.hpp](../lib/power/RPIBootManager.hpp)

### 4.3 Indicators

The board drives several visual feedback outputs:

- **Status LEDs** — activity and button status indicators.
- **Power LED** — reflects the current power state (off, sleep, on, transitioning).
- **SPI button LEDs** — 16 LEDs driven via SPI shift registers, used for per-button lighting. Overall brightness is PWM-controlled via GPIO 21 and automatically tracks the screen brightness level.
- **Monitor brightness controller** — PWM-controlled brightness for the attached display. Level 0–9 is persisted to NVS and restored on boot. Changing the level also updates the button LED brightness so both outputs stay in sync.
- **SpiBootIndicator** — flashes all SPI LEDs during the Raspberry Pi boot wait. Slow flash (~1 Hz) while waiting; fast flash (~3.3 Hz) on timeout or firmware init failure. Stops and clears on successful boot.

Relevant code lives mainly under:

- [lib/indicators](../lib/indicators)

## 5. Current Hardware Configuration Reference

The main hardware constants are centralized in:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)

### 5.1 UART And Handshake Pins On The ESP32-S3

Current firmware pin assignments:

| Function | ESP32-S3 Pin | Notes |
|---|---:|---|
| UART port | `UART_NUM_2` | Main Pi communication port |
| UART TX | `GPIO2` | ESP32 -> Pi UART data |
| UART RX | `GPIO1` | Pi -> ESP32 UART data |
| Pi data-ready input | `GPIO42` | Pi pulses this to tell ESP32 data is available |
| ESP32 data-ready output | `GPIO41` | ESP32 pulses this to tell Pi data is available |

### 5.2 Raspberry Pi Side Wiring Reference

The Raspberry Pi companion scripts currently use:

| Function | Raspberry Pi BCM | Physical Pin | Notes |
|---|---:|---:|---|
| ESP32 -> Pi data-ready | 23 | 16 | Pi listener waits for rising edge here |
| Pi -> ESP32 data-ready | 24 | 18 | Pi sender script pulses this line |
| UART5 TX | 12 | 32 | Standard `dtoverlay=uart5` mapping |
| UART5 RX | 13 | 33 | Standard `dtoverlay=uart5` mapping |

Important wiring summary:

- Pi BCM23 / physical pin 16 connects to ESP32 GPIO41.
- Pi BCM24 / physical pin 18 connects to ESP32 GPIO42.
- Pi UART5 TX connects to ESP32 GPIO1.
- Pi UART5 RX connects to ESP32 GPIO2.
- Grounds must be common between both boards.

Raspberry Pi companion references:

- [scripts/rpi/home/antho/UAart5Listener.py](../scripts/rpi/home/antho/UAart5Listener.py)
- [scripts/rpi/home/antho/heartbeat_sender.py](../scripts/rpi/home/antho/heartbeat_sender.py)
- [scripts/rpi/README.md](../scripts/rpi/README.md)

## 6. UART Protocol Summary

The firmware and Raspberry Pi use an 18-byte packet format.

At a high level, a message contains:

- a start byte,
- protocol version,
- source application ID,
- message type,
- sequence number,
- 16-bit command ID,
- five 16-bit parameters,
- checksum.

Important protocol facts:

- start byte is `0xAA`,
- packet size is `18` bytes,
- the protocol supports command, status, ACK, and NACK message types,
- heartbeat is command ID `0x0003`.

Current command IDs are defined in:

- [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp)

Examples include:

- system power,
- Raspberry Pi shutdown,
- heartbeat,
- next/previous track,
- play/pause,
- stop,
- skip forward/back,
- DAC toggle,
- display toggle,
- meter toggle,
- cover view toggle,
- rotary action.

## 7. Heartbeat Behavior

Heartbeat exists so the ESP32 can detect whether the Raspberry Pi is still alive.

Current behavior in the code:

- the ESP32 starts a heartbeat timeout monitor during initialization,
- when a heartbeat packet is received, the action processor is notified,
- if the timeout expires, the action processor is notified that the Pi is offline,
- the code currently accepts the current heartbeat command ID and also a legacy heartbeat ID `0x9999`.

Relevant references:

- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)
- [lib/transport/uart/serial.hpp](../lib/transport/uart/serial.hpp)
- [scripts/rpi/home/antho/heartbeat_sender.py](../scripts/rpi/home/antho/heartbeat_sender.py)

## 8. Build And Test Reference

### 8.1 Firmware Build

The verified workspace build path is the VS Code task:

- `PlatformIO Build`

Top-level configuration files:

- [platformio.ini](../platformio.ini)
- [CMakeLists.txt](../CMakeLists.txt)

### 8.2 Host-Side Tests

There is also a separate host-side test setup under:

- [host_tests](../host_tests)

Current note from repository context:

- host tests can be run with CMake/MSVC on this machine,
- PlatformIO native is not currently usable here without a GCC-compatible compiler in `PATH`.

### 8.3 Device Tests

PlatformIO device tests live under `test/`. They are compiled for the ESP32-S3 and require a connected board to run:

```
pio test
```

Current test suites:

| Suite | What it covers |
|---|---|
| [test/test_power_led](../test/test_power_led) | `PowerLed` constructor defaults and brightness scaling |
| [test/test_simple_command_action](../test/test_simple_command_action) | `SimpleCommandAction` press/release response |
| [test/test_uart_protocol](../test/test_uart_protocol) | UART message serialization, deserialization, checksum |
| [test/test_spi_boot_indicator](../test/test_spi_boot_indicator) | `SpiBootIndicator` state machine: start/success/failure/idempotency |
| [test/test_spi_led_driver](../test/test_spi_led_driver) | `SpiLedDriver` constructor state and early-return guard paths |

All five suites build and link cleanly against the ESP32-S3 toolchain.

## 9. Folder Guide

This is a simple description of the current project layout.

| Area | Purpose |
|---|---|
| `src/` | firmware entry point |
| `lib/board/` | board-specific constants, identity, and debug flags |
| `lib/app/` | orchestration and top-level runtime composition |
| `lib/input/buttons/` | button and input-expander handling |
| `lib/input/actions/` | action definitions and action results |
| `lib/transport/uart/` | UART transport and handshake handling |
| `lib/protocol/` | packet definitions and command IDs |
| `lib/indicators/` | LED, status, brightness behavior, and boot indication |
| `lib/power/` | power lifecycle and shutdown coordination |
| `lib/relays/` | relay abstraction |
| `lib/support/` | NVS storage and shared utilities |
| `scripts/rpi/` | Raspberry Pi listener, sender, and setup docs |
| `docs/` | project documentation |
| `host_tests/` | host-based test project (pure C++, no ESP-IDF) |
| `test/` | PlatformIO device test suites (run on ESP32-S3) |

Also relevant:

- [docs/project-structure-plan.md](./project-structure-plan.md) describes the remaining cleanup after the transport and script migration work.

## 10. Documentation Roadmap

The project now has focused documents for architecture, wiring, protocol, Raspberry Pi setup, power sequencing, and button mapping.

The next useful additions would be:

- a relay-by-relay electrical purpose reference,
- a troubleshooting matrix for heartbeat and UART failures,
- a deployment checklist for production hardware,
- photos or diagrams for the physical build.

## 11. Known Gaps And Follow-Up Topics

This section is intentionally here so the document can grow with the project.

Suggested next sections to add later:

1. Full button-to-command mapping table.
2. Power-state behavior and startup/shutdown sequencing.
3. LED behavior reference with screenshots or state tables.
4. Detailed relay ownership and electrical purpose.
5. Complete UART command catalog with sender, receiver, and expected effect.
6. Raspberry Pi service setup summary for production deployment.
7. Troubleshooting guide for wiring, heartbeat, and UART failures.

## 12. Quick Reference

If you only need the shortest summary:

- `main.cpp` starts `ControlBoard`.
- `ControlBoard` initializes the hardware and registers callbacks.
- button and rotary events become `ActionResponse` objects.
- `ActionProcessor` converts those responses into hardware actions and UART commands.
- `Serial` moves packets between the ESP32 and Raspberry Pi.
- `uartProtocol.hpp` defines what those packets look like.
- Raspberry Pi scripts in `scripts/rpi/` implement the current Pi-side listener and heartbeat sender.
