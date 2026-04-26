# TinyControlBoard Structure Plan

## Objective

The project is already moving toward a domain-based layout. The remaining problem is mixed ownership: canonical files now exist in some domain folders, while older compatibility and generic buckets still carry real behavior.

The target state is:

- one canonical location per concept,
- temporary wrappers only where needed for compatibility,
- domain folders for behavior, generic folders only for orchestration or shared support.

## Target Layout

```text
src/
  main.cpp
  main.h

lib/
  app/
    ActionCommandRoutingPolicy.hpp
    ActionUartDispatcher.cpp
    ActionUartDispatcher.hpp
    ControlBoard.cpp
    ControlBoard.hpp
    ControlBoardActionRegistry.cpp
    ControlBoardActionRegistry.hpp
    ControlBoardBootstrap.cpp
    ControlBoardBootstrap.hpp
    ControlBoardButtonIds.hpp
    ControlBoardInputDispatcher.cpp
    ControlBoardInputDispatcher.hpp
    SerialHeartbeatRouter.hpp
    SerialUartCommandSink.cpp
    SerialUartCommandSink.hpp
    actionProcessor.cpp
    actionProcessor.hpp

  board/
    boardConfig.hpp
    boardIdentity.hpp

  input/
    actions/
      actionsResponse.cpp
      actionsResponse.hpp
      actionTemplates.hpp
      buttonAction.hpp
      buttonActions.cpp
      buttonActions.hpp
      SimpleCommandAction.hpp
    buttons/
      mcpInputHandler.cpp
      mcpInputHandler.hpp
      project_cfg.hpp

  indicators/
    activityStatus.hpp
    led_definitions.hpp
    ledManager.hpp
    led_manager.cpp
    led_manager.hpp
    monitorBrightnessController.cpp
    monitorBrightnessController.hpp
    powerLed.cpp
    powerLed.hpp
    spiLedDriver.cpp
    spiLedDriver.hpp
    statusLed.cpp
    statusLed.hpp
    pwm/
      pwmLed.cpp
      pwmLed.hpp

  power/
    powerState.hpp
    PowerStateTransitionHandler.cpp
    PowerStateTransitionHandler.hpp
    PowerStateTransitionPolicy.hpp
    RelayController.cpp
    RelayController.hpp
    RPIBootManager.cpp
    RPIBootManager.hpp
    relays/
      relay.cpp
      relay.hpp

  protocol/
    uartProtocol.cpp
    uartProtocol.hpp

  transport/
    uart/
      serial.cpp
      serial.hpp
      uartReceiver.cpp
      uartReceiver.hpp

  support/
    controlSystemHelpers.cpp
    controlSystemHelpers.hpp

docs/
scripts/
  rpi/
host_tests/
test/
```

## Canonical Ownership Rules

### app/

Owns orchestration only:
- top-level wiring,
- callback registration,
- button and UART dispatch,
- composing domain services.

It should not own relay policy, boot waiting, board pin maps, or protocol constants.

### board/

Owns physical board facts:
- pins,
- ports,
- addresses,
- board identity,
- fixed timing values tied to hardware bring-up.

### input/

Owns human input and local action mapping:
- button capture,
- MCP expander details,
- rotary decoding,
- button-to-action lookup,
- reusable button action objects.

### indicators/

Owns user-visible feedback only:
- status LEDs,
- power LED behavior,
- brightness control,
- PWM LED helpers,
- SPI LED driver.

### power/

Owns power lifecycle behavior:
- relay abstraction,
- relay sequencing,
- boot and shutdown coordination,
- power-state transition policy,
- power-state types.

### protocol/

Owns wire format and command meaning:
- framing,
- packet interpretation,
- serialization,
- command IDs.

### transport/

Owns how bytes move, not what they mean:
- UART configuration,
- receive buffering,
- handshake GPIO behavior,
- transport-facing protocol adapters left over from legacy code.

### support/

Owns small shared helpers that are not themselves a subsystem.

## Exact Move Plan

### Implemented In This Pass

These ownership moves are now complete and new code should include the canonical locations directly:

| Old location | Canonical location | Status |
|---|---|---|
| `lib/controlSystem/RelayController.hpp` | `lib/power/RelayController.hpp` | wrapper removed |
| `lib/controlSystem/RelayController.cpp` | `lib/power/RelayController.cpp` | wrapper removed |
| `lib/controlSystem/RPIBootManager.hpp` | `lib/power/RPIBootManager.hpp` | wrapper removed |
| `lib/controlSystem/RPIBootManager.cpp` | `lib/power/RPIBootManager.cpp` | wrapper removed |
| `lib/controlSystem/PowerStateTransitionHandler.hpp` | `lib/power/PowerStateTransitionHandler.hpp` | wrapper removed |
| `lib/controlSystem/PowerStateTransitionHandler.cpp` | `lib/power/PowerStateTransitionHandler.cpp` | wrapper removed |
| `lib/controlSystem/PowerStateTransitionPolicy.hpp` | `lib/power/PowerStateTransitionPolicy.hpp` | wrapper removed |
| `lib/controlSystem/actionProcessor.*` | `lib/app/actionProcessor.*` | wrapper removed |
| `lib/controlSystem/ActionUartDispatcher.*` | `lib/app/ActionUartDispatcher.*` | wrapper removed |
| `lib/controlSystem/ControlBoard*` | `lib/app/ControlBoard*` | wrapper removed |
| `lib/controlSystem/SerialUartCommandSink.*` | `lib/app/SerialUartCommandSink.*` | wrapper removed |
| `lib/actions/*` | `lib/input/actions/` | wrapper removed |
| `lib/buttons/*` | `lib/input/buttons/` | wrapper removed |

Current note:

- The firmware build now compiles the canonical `lib/app`, `lib/input`, `lib/power`, `lib/protocol`, `lib/support`, and `lib/transport/uart` sources directly.
- The remaining migration work is any deliberate cleanup of still-unused legacy helpers and any naming cleanup you still want to do.

### Next File Moves

| Current file or folder | Target | Reason |
|---|---|---|
| `legacy helper headers if reintroduced` | subsystem-owned canonical headers | avoid recreating compatibility layers |
| `new transport aliases` | `lib/transport/uart/` only | keep one transport surface |
| `new Pi deployment notes` | `scripts/rpi/README.md` | keep deployment guidance close to the tracked assets |

## Migration Sequence

1. Move ownership first, names second.
2. Keep compatibility headers or translation-unit wrappers until all includes and build discovery are stable.
3. Update new code to include canonical paths immediately after each move.
4. After one area is fully migrated, delete its wrappers in one follow-up change.
5. Rename mixed-case or misspelled files only after ownership has stabilized.

## Naming Cleanup Queue

These are worthwhile, but should happen after ownership cleanup rather than during it:

- consider `controlSystem/` -> `app/`

## Test Layout Recommendation

Keep both test areas, but make the distinction explicit:

- `host_tests/` for host-side CMake logic tests,
- `test/` for embedded or PlatformIO-targeted tests.

If you want the tree to read more cleanly later, move them to:

- `tests/host/`
- `tests/embedded/`

## Exit Criteria

The folder cleanup is done when all of the following are true:

- every subsystem has exactly one canonical folder,
- `common/` contains only temporary wrappers or is gone,
- `app/` remains the only orchestration folder,
- `transport/uart/` remains the only transport surface,
- `lib/protocol/uartProtocol.*` remains the only protocol surface,
- visual code is entirely under `indicators/`,
- Pi helper scripts are no longer stored under `lib/`.
