# TinyControlBoard Structure Plan

## Goals

- Keep cross-cutting constants in one place.
- Keep hardware wiring separate from protocol definitions.
- Keep module-private constants inside the module that owns them.
- Reduce file-name and include-path ambiguity.

## Recommended Layout

```text
lib/
  app/
    ControlBoard.cpp
    ControlBoard.hpp
    Startup.cpp
    Startup.hpp
  board/
    boardConfig.hpp
    boardIdentity.hpp
  protocol/
    uartProtocol.cpp
    uartProtocol.hpp
    commandIds.hpp
  input/
    mcpInputHandler.cpp
    mcpInputHandler.hpp
    buttonActions.cpp
    buttonActions.hpp
    actionTemplates.hpp
  indicators/
    activeLed.cpp
    activeLed.hpp
    ledDefinitions.hpp
    ledManager.cpp
    ledManager.hpp
    monitorBrightnessController.cpp
    monitorBrightnessController.hpp
    powerLed.cpp
    powerLed.hpp
    spiLedDriver.cpp
    spiLedDriver.hpp
  power/
    relay.cpp
    relay.hpp
    relayController.cpp
    relayController.hpp
    rpiBootManager.cpp
    rpiBootManager.hpp
    powerState.hpp
  support/
    actionProcessor.cpp
    actionProcessor.hpp
    controlSystemHelpers.cpp
    controlSystemHelpers.hpp
```

## What Should Live Where

### board/

Owns physical board facts:
- GPIO assignments
- I2C addresses
- UART port selection
- boot delays and timeouts tied to the board
- board identity strings

Current candidates:
- lib/common/boardConfig.hpp
- lib/common/GlobalDefines.hpp
- relay pin mapping currently exposed by lib/relays/relay.hpp
- LED and SPI pin mapping currently in lib/indicators/led_manager.cpp

### protocol/

Owns wire-format facts:
- frame constants
- packet indices
- command IDs
- source application IDs
- message serialization/deserialization

Current candidates:
- lib/serialBus/uart_protocol.hpp
- lib/serialBus/uart_protocol.cpp

Rule: no protocol constant should be duplicated outside protocol/.

### input/

Owns buttons, rotary input, and action binding:
- MCP register constants
- button-to-action mapping
- rotary decoding
- action templates and concrete button actions

Current candidates:
- lib/buttons/*
- lib/actions/*

### indicators/

Owns LED and display feedback only:
- LED PWM constants
- active/power/button LED behavior
- SPI LED driver
- monitor brightness control

Current candidates:
- lib/indicators/*
- lib/led/*

### power/

Owns relay and power-state transitions:
- relay abstraction
- relay sequencing
- RPI boot/shutdown coordination
- power state enum

Current candidates:
- lib/relays/*
- lib/common/PowerStateManager.hpp
- lib/controlSystem/relayController.*
- lib/controlSystem/rpiBootManager.*

### app/ and support/

Owns orchestration, not raw constants:
- startup order
- module wiring
- callbacks
- translation from input events to protocol/power actions

Current candidates:
- lib/controlSystem/ControlBoard.*
- lib/controlSystem/actionProcessor.*
- lib/controlSystem/controlSystemHelpers.*

## Migration Order

1. Keep lib/common/boardConfig.hpp as the only source of board wiring.
2. Move uart_protocol files under a dedicated protocol folder and stop exporting protocol values through GlobalDefines.
3. Rename mixed-case and misspelled files to canonical names and delete the temporary shim headers.
4. Collapse overlapping state enums into one power-state model and one activity-status model.
5. Move relay and boot management out of controlSystem into a dedicated power area.
6. Move button and action code under one input area so button mapping is easier to find.

## Immediate Follow-Up Candidates

- Replace legacy macros in protocol headers with constexpr values or scoped enums.
- Rename led_Manager.hpp and controlSytemHelpers.hpp on disk after all includes use the canonical names.
- Convert board identity macros to constexpr-only usage.
- Consider replacing generic names like common and controlSystem with domain names that describe ownership.
