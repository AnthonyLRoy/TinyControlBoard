# TinyControlBoard Architecture

This document explains how the current firmware is structured and how control flows through the system.

## 1. High-Level View

The firmware is built around one top-level orchestrator: `ControlBoard`.

At runtime, the system does four main jobs:

1. initialize the board and connected hardware,
2. read user input from buttons and rotary input,
3. convert those inputs into local actions or UART commands,
4. monitor the Raspberry Pi link and react to heartbeat state.

Core entry points:

- [src/main.cpp](../src/main.cpp)
- [lib/app/ControlBoard.hpp](../lib/app/ControlBoard.hpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)

## 2. Startup Flow

Current startup sequence:

1. `app_main()` waits 5 seconds for power to settle.
2. A `ControlBoard` instance is created.
3. `ControlBoard::init()` sets initial LED state and brightness.
4. The serial singleton and relay singleton are acquired.
5. The serial receive callback is registered.
6. Heartbeat monitoring is started.
7. The action processor is created.
8. Relays are initialized and set to default off states.
9. The MCP input handler is initialized.
10. Button and rotary callbacks are registered.
11. UART is initialized.
12. The button-to-action map is created.
13. After `kInitDelayMs`, the power LED is moved to `SLEEP`.

References:

- [src/main.cpp](../src/main.cpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)
- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)

## 3. Main Components

### 3.1 `ControlBoard`

`ControlBoard` is the integration layer. It owns callback registration and connects the input, serial, relay, and action-processing subsystems.

Main responsibilities:

- initialize relays,
- initialize UART and serial callbacks,
- initialize the MCP input handler,
- route button press, button release, and rotary events,
- route incoming UART messages,
- maintain the button-action lookup table.

References:

- [lib/app/ControlBoard.hpp](../lib/app/ControlBoard.hpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)

### 3.2 `ActionProcessor`

`ActionProcessor` takes an `ActionResponse` and decides what side effect should happen.

That can include:

- local relay changes,
- local brightness changes,
- sending UART commands to the Raspberry Pi,
- handling power-state transitions,
- waiting for Raspberry Pi heartbeat during boot/shutdown coordination.

Reference:

- [lib/app/actionProcessor.hpp](../lib/app/actionProcessor.hpp)
- [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp)

### 3.3 `Serial`

The serial layer owns the ESP32 side of the Pi link.

It does three distinct jobs:

- configure and use UART2,
- manage the two extra data-ready handshake GPIOs,
- track heartbeat timing and pass completed messages up through a callback.

References:

- [lib/transport/uart/serial.hpp](../lib/transport/uart/serial.hpp)
- [lib/transport/uart/serial.cpp](../lib/transport/uart/serial.cpp)

### 3.4 `RpiBootManager`

`RpiBootManager` is a synchronization helper built around a FreeRTOS event group.

It does not directly control hardware. Instead, it waits for lifecycle signals:

- heartbeat received means the Pi is alive or has finished booting,
- heartbeat timeout is treated as shutdown/offline confirmation.

References:

- [lib/power/RPIBootManager.hpp](../lib/power/RPIBootManager.hpp)
- [lib/power/RPIBootManager.cpp](../lib/power/RPIBootManager.cpp)

### 3.5 `RelayController`

`RelayController` is a thin helper around the relay abstraction. It currently provides:

- relay state changes with optional delay,
- DAC relay toggling,
- Raspberry Pi relay shutdown,
- screen relay shutdown.

References:

- [lib/power/RelayController.hpp](../lib/power/RelayController.hpp)
- [lib/power/RelayController.cpp](../lib/power/RelayController.cpp)

## 4. Input Flow

The current input path is:

1. MCP input or rotary movement is detected.
2. `ControlBoard` callback receives the event.
3. The button index is used to find a `ButtonAction` object.
4. The action returns an `ActionResponse`.
5. `ActionProcessor::process()` applies the result.

Related files:

- [lib/input/buttons/mcpInputHandler.hpp](../lib/input/buttons/mcpInputHandler.hpp)
- [lib/input/buttons/mcpInputHandler.cpp](../lib/input/buttons/mcpInputHandler.cpp)
- [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp)
- [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp)
- [lib/input/actions/actionTemplates.hpp](../lib/input/actions/actionTemplates.hpp)
- [lib/input/actions/actionsResponse.hpp](../lib/input/actions/actionsResponse.hpp)

## 5. UART Flow

The current ESP32 -> Pi path is:

1. An action processor branch decides to send a command.
2. A `UartMessage` or command ID is passed to `Serial`.
3. `Serial::sendData()` writes bytes on UART.
4. The ESP32 raises its data-ready pin briefly so the Pi knows to read.

The Pi -> ESP32 path is:

1. The Raspberry Pi writes a UART packet.
2. The Pi pulses its data-ready line.
3. The ESP32 ISR wakes the UART RX task.
4. Incoming bytes are added to the receiver buffer.
5. Complete messages are deserialized and passed to the registered callback.

Important current limitation:

- `ControlBoard::handleSerialRxMessage()` actively handles heartbeat messages.
- Other received UART messages are logged but are not yet processed into local behavior.

References:

- [lib/transport/uart/serial.cpp](../lib/transport/uart/serial.cpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)
- [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp)

## 6. State And Status Model

### 6.1 Power State

The main power LED state model currently includes:

- `OFF`
- `TURNING_ON`
- `ON`
- `SLEEP`
- `GOING_TO_SLEEP`
- `DEEPSLEEP`

Reference:

- [lib/power/powerState.hpp](../lib/power/powerState.hpp)

### 6.2 Working Status

The activity/status LED model currently includes:

- `doingWork`
- `Idle`
- `SolidIdle`
- `sleeping`
- `MaintenanceMode`
- `Active`

Reference:

- [lib/indicators/activityStatus.hpp](../lib/indicators/activityStatus.hpp)

## 7. Folder Ownership

This is the practical ownership model for the current codebase.

| Area | Current Role |
|---|---|
| `src/` | firmware entry point |
| `lib/board/` | board constants and identity |
| `lib/app/` | orchestration and integration layer |
| `lib/input/actions/` | reusable button action objects and templates |
| `lib/input/buttons/` | input expander and input capture |
| `lib/transport/uart/` | UART transport and handshake logic |
| `lib/protocol/` | wire format and command IDs |
| `lib/indicators/` | LEDs, status behavior, brightness control |
| `lib/power/` | power state, relay sequencing, and Pi boot/shutdown coordination |
| `lib/relays/` | low-level relay abstraction |
| `scripts/rpi/` | Raspberry Pi listener, sender, and setup docs |

## 8. Notable Current Design Characteristics

- `ControlBoard` is the integration hub and currently owns a lot of orchestration.
- Input actions are represented as objects, which makes it straightforward to remap buttons without rewriting processor logic.
- Toggle actions maintain internal software state, so their first emitted command depends on the starting state in firmware.
- Heartbeat is treated as the Raspberry Pi liveness signal for both boot completion and shutdown detection.
- Non-heartbeat Pi-originated commands are not yet fully consumed on the ESP32 side.

## 9. Related Docs

- [docs/project-guide.md](./project-guide.md)
- [docs/button-command-map.md](./button-command-map.md)
- [docs/power-sequencing.md](./power-sequencing.md)
- [docs/wiring-reference.md](./wiring-reference.md)
- [docs/protocol-reference.md](./protocol-reference.md)
- [docs/raspberry-pi-setup.md](./raspberry-pi-setup.md)