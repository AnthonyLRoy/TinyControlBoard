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
3. NVS flash is initialized.
4. `bootstrap::prepareStartupIndicators()` sets the power LED to `TURNING_ON`.
5. The UART and relay singletons are acquired; serial RX and heartbeat-timeout callbacks are registered.
6. `ActionProcessor` and `ControlBoardInputDispatcher` are constructed.
7. Relays are initialized and set to default off states.
8. The MCP input handler is initialized.
9. The `ButtonEventQueue` task is started.
10. Button press, release, and rotary callbacks are registered.
11. UART hardware is initialized.
12. The button-to-action map is populated via `ControlBoardActionRegistry`.
13. After a `board::timing::k_initDelayMs` delay, `triggerInitialPowerOn()` is called which starts the full power-on sequence.

Note: if any critical init step fails (MCP handler or serial setup), `SpiBootIndicator::notifyFailure()` is called immediately, which flashes all SPI LEDs rapidly to signal the fault before the firmware aborts init.

References:

- [src/main.cpp](../src/main.cpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)
- [lib/app/ControlBoardBootstrap.cpp](../lib/app/ControlBoardBootstrap.cpp)
- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)

## 3. Main Components

### 3.1 `ControlBoard`

`ControlBoard` is the top-level integration class. It owns the component instances and routes events. Initialization logic is split into helper namespaces:

- `bootstrap::` functions (in `ControlBoardBootstrap.cpp`) handle startup step sequencing.
- `ControlBoardActionRegistry` populates the button-to-action map.
- `ControlBoardInputDispatcher` translates button/rotary events into `IAction` objects and manages per-button LED state.
- `ButtonEventQueue` serializes press, release, and rotary events onto a FreeRTOS queue processed by `ControlBoardInputDispatcher`.

Main responsibilities:

- initialize NVS, relays, UART, and serial callbacks,
- initialize the MCP input handler,
- start the `ButtonEventQueue` and register MCP callbacks,
- populate the button-to-action registry,
- trigger the initial power-on sequence.

References:

- [lib/app/ControlBoard.hpp](../lib/app/ControlBoard.hpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)
- [lib/app/ControlBoardBootstrap.hpp](../lib/app/ControlBoardBootstrap.hpp)
- [lib/app/ControlBoardActionRegistry.hpp](../lib/app/ControlBoardActionRegistry.hpp)
- [lib/app/ControlBoardInputDispatcher.hpp](../lib/app/ControlBoardInputDispatcher.hpp)
- [lib/app/ButtonEventQueue.hpp](../lib/app/ButtonEventQueue.hpp)

### 3.2 `ActionProcessor`

`ActionProcessor` receives a `std::unique_ptr<actions::IAction>` and executes it via the `ActionContext` services struct.

Side effects are implemented in concrete `IAction` subclasses under `lib/app/commands/`:

- `RelayAction` — local relay changes,
- `BrightnessAction` — local brightness changes,
- `UartDispatchAction` — sends UART commands to the Raspberry Pi,
- `PowerTransitionAction` — handles power-state transitions,
- `DisplayAction` — display on/off control,
- `SystemAction` — system-level commands.

`ActionFactory::createAction()` maps a `CommandId` to the appropriate concrete type via `ActionCommandRoutingPolicy`.

References:

- [lib/app/actionProcessor.hpp](../lib/app/actionProcessor.hpp)
- [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp)
- [lib/app/ActionFactory.hpp](../lib/app/ActionFactory.hpp)
- [lib/app/ActionContext.hpp](../lib/app/ActionContext.hpp)
- [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp)
- [lib/app/commands/](../lib/app/commands/)

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

Debug flag:

- `board::debug::kSimulateRpiBoot` in [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp) can be set to `true` to skip the heartbeat wait entirely. This is useful when testing on the bench without a Raspberry Pi connected. The flag is evaluated at compile time (`if constexpr`) so there is zero overhead in release builds.

References:

- [lib/power/RPIBootManager.hpp](../lib/power/RPIBootManager.hpp)
- [lib/power/RPIBootManager.cpp](../lib/power/RPIBootManager.cpp)
- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)

### 3.5 `SpiBootIndicator`

`SpiBootIndicator` is a FreeRTOS-based visual boot indicator that flashes all 16 SPI-driven button LEDs to signal boot progress or failure.

Behavior:

- `startWaiting()` — starts a background FreeRTOS task that flashes all LEDs at 500 ms half-period (1 Hz) while the firmware waits for the Raspberry Pi heartbeat.
- `notifySuccess()` — signals the task to stop and clears all LEDs. Called when the heartbeat is received within the timeout.
- `notifyFailure()` — switches to a fast 150 ms half-period flash (~3.3 Hz). If the task is not yet running (firmware init failure), it starts the task directly in the failed state.

Call sites:

- `PowerStateTransitionHandler` calls `startWaiting()` just before `waitForRpiToBoot()`, then calls `notifySuccess()` or `notifyFailure()` based on the result.
- `ControlBoard::init()` calls `notifyFailure()` before each early-return failure path.

The singleton accessor is `indicators::getSpiBootIndicator()`, registered in `led_manager.cpp`.

References:

- [lib/indicators/SpiBootIndicator.hpp](../lib/indicators/SpiBootIndicator.hpp)
- [lib/indicators/SpiBootIndicator.cpp](../lib/indicators/SpiBootIndicator.cpp)
- [lib/indicators/ledManager.hpp](../lib/indicators/ledManager.hpp)
- [lib/indicators/ledManager.cpp](../lib/indicators/ledManager.cpp)

### 3.6 `SpiLedDriver`

`SpiLedDriver` drives 16 button LEDs via SPI shift registers (SPI2_HOST, 1 MHz, GPIO 6/7/5 for clock/MOSI/latch).

Key methods:

- `init()` — sets up the SPI bus and device.
- `setLed(index, on)` — sets a single LED by bit index (0–15).
- `setAllLeds(on)` — atomically sets all 16 LEDs on or off. Used by `SpiBootIndicator` for whole-panel flashing.
- `update()` — pushes the current 16-bit state to the shift register via SPI.

The overall brightness of all button LEDs is controlled by a PWM duty on GPIO 21 (`k_buttonLedPwmPin`) through the `StatusLed` instance registered as `s_buttonStatusLed`. The duty is updated by `MonitorBrightnessController` whenever screen brightness changes so the two track together (see §3.7).

References:

- [lib/hal/leds/spiLedDriver.hpp](../lib/hal/leds/spiLedDriver.hpp)
- [lib/hal/leds/spiLedDriver.cpp](../lib/hal/leds/spiLedDriver.cpp)

### 3.7 `MonitorBrightnessController` And Button LED Coupling

`MonitorBrightnessController` owns the PWM duty on the monitor brightness pin and also drives the button LED brightness level. Every method that changes screen brightness calls `getButtonStatusLed().setIdleDuty()` with an inverted mapping:

- level 0 (screen dimmest) → button LEDs dimmest,
- level 9 (screen brightest) → button LEDs brightest.

The inversion is applied inside `buttonDutyForLevel()` in `MonitorBrightnessController.cpp` because the button LED circuit is active-low (higher LEDC duty → dimmer output).

`setIdleDuty()` on `StatusLed` is thread-safe (`std::atomic<uint32_t> m_idleDuty`) and takes effect immediately when the LED task is in `SolidIdle` state.

The coupling is active through:

- `init()` — applies the NVS-restored level to both outputs on startup,
- `changeBrightnessLevel()` / `cycleBrightness()` — user brightness steps,
- `setState()` ON path — restores saved level on wake,
- `toggleDisplayOffOn()` — drives both to zero when display-off is active, restores on toggle-off,
- `clearDisplayOffMode()` — restores both before a power transition.

References:

- [lib/indicators/monitorBrightnessController.hpp](../lib/indicators/monitorBrightnessController.hpp)
- [lib/indicators/MonitorBrightnessController.cpp](../lib/indicators/MonitorBrightnessController.cpp)
- [lib/indicators/statusLed.hpp](../lib/indicators/statusLed.hpp)

### 3.8 `RelayController`

`RelayController` is a thin helper around the relay abstraction. It currently provides:

- relay state changes with optional delay,
- DAC relay toggling,
- Raspberry Pi relay shutdown,
- screen relay shutdown.

`StandardRelay::setRelayState()` returns a `bool` — `true` if `gpio_set_level()` succeeded, `false` on driver error (e.g. pin not configured or invalid pin number). Physical contact state is not detectable without dedicated feedback hardware.

References:

- [lib/power/RelayController.hpp](../lib/power/RelayController.hpp)
- [lib/power/RelayController.cpp](../lib/power/RelayController.cpp)
- [lib/hal/relay/relay.hpp](../lib/hal/relay/relay.hpp)
- [lib/hal/relay/relay.cpp](../lib/hal/relay/relay.cpp)

## 4. Input Flow

The current input path is:

1. MCP interrupt fires; `McpInputHandler` decodes it as a press, release, or rotary event.
2. The event is enqueued into `ButtonEventQueue`.
3. `ControlBoardInputDispatcher` dequeues the event and looks up the `ButtonConfig` for that button index.
4. `ButtonConfig::action->produce(isPressed)` is called on the `IActionSource` to obtain a `std::unique_ptr<actions::IAction>`.
5. `ControlBoardInputDispatcher` applies `LedPolicy` (None / Momentary / Toggle) to the SPI LED state for that button.
6. The `IAction` is passed to `ActionProcessor::process()`.
7. `ActionProcessor` builds an `ActionContext` and calls `iaction->execute(ctx)`.
8. The concrete command class performs the side effect (relay, UART, brightness, power transition, etc.).

Related files:

- [lib/hal/buttons/mcpInputHandler.hpp](../lib/hal/buttons/mcpInputHandler.hpp)
- [lib/hal/buttons/mcpInputHandler.cpp](../lib/hal/buttons/mcpInputHandler.cpp)
- [lib/app/ButtonEventQueue.hpp](../lib/app/ButtonEventQueue.hpp)
- [lib/app/ControlBoardInputDispatcher.hpp](../lib/app/ControlBoardInputDispatcher.hpp)
- [lib/input/actions/actionTemplates.hpp](../lib/input/actions/actionTemplates.hpp)
- [lib/input/actions/IAction.hpp](../lib/input/actions/IAction.hpp)
- [lib/app/ActionFactory.hpp](../lib/app/ActionFactory.hpp)
- [lib/app/ActionContext.hpp](../lib/app/ActionContext.hpp)

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
- Other received UART messages are now routed through a minimal `ActionProcessor` inbound scaffold and logged, but protocol-specific behavior is still pending.

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
| `lib/board/` | board constants, identity, and debug flags |
| `lib/app/` | orchestration and integration layer |
| `lib/input/actions/` | reusable button action objects and templates |
| `lib/hal/buttons/` | MCP23017 input expander driver |
| `lib/hal/leds/` | SPI and PWM LED drivers |
| `lib/hal/relay/` | low-level relay GPIO abstraction |
| `lib/hal/uart/` | UART transport and handshake logic |
| `lib/hal/storage/` | NVS storage helper |
| `lib/protocol/` | wire format and command IDs |
| `lib/indicators/` | LED services, brightness control, and boot indication |
| `lib/power/` | power state, relay sequencing, and Pi boot/shutdown coordination |
| `scripts/rpi/` | Raspberry Pi listener, sender, and setup docs |
| `host_tests/` | pure C++ host-side tests (no ESP-IDF required) |
| `test/` | PlatformIO device tests (run on connected ESP32-S3) |

## 8. Notable Current Design Characteristics

- `ControlBoard` is the integration hub and currently owns a lot of orchestration.
- Input actions are represented as objects, which makes it straightforward to remap buttons without rewriting processor logic.
- Toggle actions maintain internal software state, so their first emitted command depends on the starting state in firmware.
- Heartbeat is treated as the Raspberry Pi liveness signal for both boot completion and shutdown detection.
- Non-heartbeat Pi-originated commands are not yet fully consumed on the ESP32 side.
- Boot and init failures are signalled visually via `SpiBootIndicator`: slow flashing (~1 Hz) during normal boot wait, fast flashing (~3.3 Hz) on timeout or firmware init failure.
- The `board::debug::kSimulateRpiBoot` compile-time flag allows full firmware testing without a connected Raspberry Pi. When `true`, the 60-second heartbeat wait is skipped instantly.
- Button LED brightness tracks monitor brightness automatically. `MonitorBrightnessController` calls `getButtonStatusLed().setIdleDuty()` at every brightness-change site using an inverted active-low mapping.
- `StandardRelay::setRelayState()` returns a bool indicating GPIO driver success. Physical relay contact state cannot be detected without additional feedback hardware (current sense or optocoupler on the switched output).

## 9. Related Docs

- [docs/project-guide.md](./project-guide.md)
- [docs/button-command-map.md](./button-command-map.md)
- [docs/power-sequencing.md](./power-sequencing.md)
- [docs/wiring-reference.md](./wiring-reference.md)
- [docs/protocol-reference.md](./protocol-reference.md)
- [docs/raspberry-pi-setup.md](./raspberry-pi-setup.md)