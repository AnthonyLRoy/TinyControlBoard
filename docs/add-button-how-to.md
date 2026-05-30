# How To Add A New Button Action

This guide shows the current firmware path for adding another button-driven action.

It covers three common cases:

- a button that sends a simple UART command to the Raspberry Pi,
- a button that sends a UART message with parameters,
- a button that stays local on the ESP32 and does not send UART.

The current canonical ownership is:

- button IDs and hardware pin constants: [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
- control-board button constants: [lib/app/ControlBoardButtonIds.hpp](../lib/app/ControlBoardButtonIds.hpp)
- action object declarations: [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp)
- action object definitions: [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp)
- button-to-action map: [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp)
- command routing and side effects: [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp)
- UART translation layer: [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp)
- protocol command IDs: [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp)

Do not add new work to the legacy compatibility wrappers under `lib/controlSystem`, `lib/actions`, or `lib/buttons` unless you are intentionally maintaining backward compatibility. The canonical implementation lives in the files listed above.

## 1. Decide What Kind Of Button You Are Adding

Before editing code, decide which of these paths matches the behavior you want.

### 1.1 Simple UART Command

Use this when one press should produce one command ID and send it to the Raspberry Pi.

Examples already in the codebase:

- next track,
- play/pause,
- next menu,
- cycle brightness if it were remote instead of local.

Typical action type:

- `SimpleCommandAction` in [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp)
- or `SimpleCommandAction` from [lib/input/actions/SimpleCommandAction.hpp](../lib/input/actions/SimpleCommandAction.hpp)

### 1.2 Parameterized UART Command

Use this when the button action needs more than a raw command ID, such as a toggle state or a direction value.

Examples already in the codebase:

- cover view toggle,
- meter toggle,
- rotary direction.

Typical action type:

- `ToggleAction<CMD_ON, CMD_OFF>`
- `RotaryAction<CMD_ROTARY_ACTION>`

These actions usually need a translation step in [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp) so the final UART packet can carry parameters.

### 1.3 Local-Only Action

Use this when the button should affect hardware or local firmware state without sending anything to the Raspberry Pi.

Examples already in the codebase:

- DAC relay toggle,
- display blank/unblank,
- brightness cycling,
- power state transitions.

These actions still produce an `ActionResponse`, but the side effect happens in [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp) rather than in the UART dispatcher.

## 2. Step-By-Step: Add The Command ID First

If the behavior needs a new command ID, define it first in [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp).

Steps:

1. Pick a command name consistent with the current naming scheme, such as `CMD_FOO_BAR`.
2. Pick a unique numeric value that does not collide with existing commands.
3. If the Raspberry Pi is involved, update the Pi-side listener script so it recognizes the new command.
4. If the command should appear in docs, update [docs/protocol-reference.md](./protocol-reference.md) and later update [docs/button-command-map.md](./button-command-map.md).

Rule of thumb:

- if the command is purely local and never crosses UART, it can still use a `CMD_*` identifier, because the action routing layer classifies commands by meaning, not only by transport.

## 3. Step-By-Step: Add Or Reserve A Button ID

If you are adding a brand-new physical button, reserve its ID in both of these files:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
- [lib/app/ControlBoardButtonIds.hpp](../lib/app/ControlBoardButtonIds.hpp)

Steps:

1. Add a new constant in `board::buttons`, for example `kMute = 16`.
2. Add the matching constant in `controlSystem::controlBoardButtons`, using the same numeric value.
3. Increase `kCount` if the total button count changed.
4. If the new button corresponds to a real MCP input or LED position, make sure the wiring and caller assumptions still hold.

Important note:

- the current button LED logic uses the button ID as the SPI LED index for non-power buttons, so changing indices also changes LED bit positions.

## 4. Step-By-Step: Create The Action Object

Define the action object in the canonical input action files.

Files:

- declarations: [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp)
- definitions: [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp)

Choose the smallest action type that matches the behavior.

### 4.1 For A Simple UART Command

In [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp), add a `SimpleCommandAction` instance:

```cpp
SimpleCommandAction MuteInstance(CMD_MUTE);
```

In [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp), declare it:

```cpp
extern SimpleCommandAction MuteInstance;
```

### 4.2 For A Toggle Or Timed Action

In [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp), add a type alias if needed:

```cpp
using ToggleMute = ToggleAction<CMD_MUTE_ON, CMD_MUTE_OFF>;
extern ToggleMute ToggleMuteInstance;
```

In [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp), define it:

```cpp
ToggleMute ToggleMuteInstance;
```

### 4.3 For A Local-Only Action

You still create an action object the same way. The difference is where it is handled later.

Example pattern:

```cpp
SimpleCommandAction ToggleFanInstance(CMD_TOGGLE_FAN);
```

## 5. Step-By-Step: Add The Button To The Registry

Bind the physical button ID to the action instance in [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp).

Steps:

1. Find `ControlBoardActionRegistry::populate(...)`.
2. Add a map entry using the canonical button ID.
3. Point it at the action object you created and choose a `LedPolicy`.

Example:

```cpp
rActionMap[controlBoardButtons::k_mute] = {&actions::MuteInstance, LedPolicy::Momentary};
```

`LedPolicy` controls the SPI LED feedback for that button:

- `LedPolicy::None` — no LED feedback (e.g. power button, rotary).
- `LedPolicy::Momentary` — LED on while held, off on release.
- `LedPolicy::Toggle` — LED state flips on each press.

At this point, the input side knows which action to execute when that button is pressed.

## 6. Step-By-Step: Wire The Behavior

This is where the three paths split.

### 6.1 Path A: Simple UART Command

If the action returns a command ID that should be sent directly as UART, update [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp).

Steps:

1. Add an entry to `commandConfigs[]`.
2. Choose a short log tag string used for UART logging.
3. Make sure the command is classified as `UartDispatch` by the routing policy.

Example:

```cpp
{"MUTE", CMD_MUTE}
```

If the command is already routed to `ActionCommandRoute::UartDispatch`, `handleSimpleCommand()` will send it automatically.

### 6.2 Path B: UART Message With Parameters

If the action needs parameters, add a dedicated handler in [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp).

Current examples to copy:

- `handleCoverViewCommand(...)`
- `handleMeterCommand(...)`
- `handleRotaryCommand(...)`

Steps:

1. Add a small handler that recognizes the action response command.
2. Build a `UartMessage`.
3. Fill `commandId` and any `params[]` entries.
4. Send it through `mrUartCommandSink.sendUartMessage(...)`.
5. Call that handler from `ActionUartDispatcher::handle(...)`.

Use this path when a raw `sendUartCommand()` would lose necessary state.

### 6.3 Path C: Local-Only Action

Add a new concrete command class under [lib/app/commands/](../lib/app/commands/).

Steps:

1. Add a classification case for the new command in [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp).
2. Add the new `ActionCommandRoute` enum value to [lib/app/ActionCommandRoute.hpp](../lib/app/ActionCommandRoute.hpp).
3. Create a `.hpp` / `.cpp` pair in `lib/app/commands/` and implement `execute(ActionContext &ctx)` to perform the local side effect.
4. Add the new route case to `ActionFactory::createAction()` in [lib/app/ActionFactory.cpp](../lib/app/ActionFactory.cpp) so the factory instantiates the new type.

Current local command examples to copy:

- `RelayAction` — [lib/app/commands/RelayAction.cpp](../lib/app/commands/RelayAction.cpp)
- `DisplayAction` — [lib/app/commands/DisplayAction.cpp](../lib/app/commands/DisplayAction.cpp)
- `BrightnessAction` — [lib/app/commands/BrightnessAction.cpp](../lib/app/commands/BrightnessAction.cpp)
- `PowerTransitionAction` — [lib/app/commands/PowerTransitionAction.cpp](../lib/app/commands/PowerTransitionAction.cpp)

For a local-only button, the usual shape is:

1. action object emits `CMD_*`,
2. routing policy classifies that command as local (a new or existing route value),
3. `ActionFactory` creates the matching concrete command,
4. `IAction::execute(ctx)` performs the hardware or state change,
5. no UART packet is sent.

## 7. Step-By-Step: Check Routing Classification

If the new command is not behaving correctly, the first thing to verify is its route classification.

The decision point is the routing layer used by [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp), via [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp).

You will typically want one of these outcomes:

- `UartDispatch` for remote commands,
- `Relay` for local relay changes,
- `Display` for local display actions,
- `Brightness` for local brightness changes,
- `PowerStateTransition` for power-button-style behavior,
- `System` for system-level commands.

If a new command is silently ignored or sent over the wrong path, this classification is usually the reason.

## 8. Step-By-Step: Update The Raspberry Pi Side When Needed

If the new button sends UART, update the corresponding Pi-side logic.

Typical files:

- [scripts/rpi/home/antho/UAart5Listener.py](../scripts/rpi/home/antho/UAart5Listener.py)
- any Pi-side application code that reacts to the command

Steps:

1. Add support for the new command ID.
2. If parameters are used, document and decode them consistently.
3. Keep the firmware-side name, numeric ID, and parameter meaning aligned.

## 9. Step-By-Step: Update The Docs

Once the button works, update the two reference docs.

Files:

- [docs/button-command-map.md](./button-command-map.md)
- [docs/protocol-reference.md](./protocol-reference.md)

Update:

1. button index,
2. action type,
3. raw command ID,
4. final routed command,
5. final effect,
6. whether it is UART or local.

## 10. Worked Examples

### 10.1 Example: Add A Simple UART Button

Goal:

- add a `Mute` button that sends `CMD_MUTE` to the Raspberry Pi.

Files to touch:

1. [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp)
2. [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
3. [lib/app/ControlBoardButtonIds.hpp](../lib/app/ControlBoardButtonIds.hpp)
4. [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp)
5. [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp)
6. [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp)
7. [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp)
8. Pi-side listener file
9. [docs/button-command-map.md](./button-command-map.md)
10. [docs/protocol-reference.md](./protocol-reference.md)

### 10.2 Example: Add A Local Relay Button

Goal:

- add a `Toggle Fan` button that flips a relay on the ESP32 and never sends UART.

Files to touch:

1. [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp)
2. [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
3. [lib/app/ControlBoardButtonIds.hpp](../lib/app/ControlBoardButtonIds.hpp)
4. [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp)
5. [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp)
6. [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp)
7. routing policy if needed
8. [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp)
9. [docs/button-command-map.md](./button-command-map.md)

## 11. Quick Checklist

Use this checklist when adding a button:

1. Add or confirm the `CMD_*` ID.
2. Add or confirm the physical/control-board button ID.
3. Declare and define the action object.
4. Register the button in `ControlBoardActionRegistry`.
5. Route it to either UART dispatch or local handling.
6. Update Pi-side handling if UART is involved.
7. Update the button and protocol docs.
8. Run a firmware build and verify the expected side effect.

## 12. Validation

The normal validation step for this repo is the `PlatformIO Build` task from the workspace root.

If the button is UART-backed, also verify:

- the log tag exists in `ActionUartDispatcher`,
- the Raspberry Pi side understands the command,
- any parameter encoding is documented.

If the button is local-only, verify:

- the routing policy sends it to the local handler,
- no accidental UART command is emitted,
- the correct relay, indicator, or state change actually occurs.