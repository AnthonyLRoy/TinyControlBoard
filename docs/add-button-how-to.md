# How To Add A New Button Action

This guide shows the current firmware path for adding another button-driven action.

It covers three common cases:

- a button that sends a simple UART command to the Raspberry Pi,
- a button that sends a UART message with parameters (e.g. a toggle that carries ON/OFF state),
- a button that stays local on the ESP32 and does not send UART.

The current canonical ownership is:

| Concern | File |
|---|---|
| Physical button ID constants | [lib/board/boardButtonIds.hpp](../lib/board/boardButtonIds.hpp) |
| Button ID namespace alias | [lib/app/ControlBoardButtonIds.hpp](../lib/app/ControlBoardButtonIds.hpp) |
| Button registration table | [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp) |
| Action source templates | [lib/input/actions/actionTemplates.hpp](../lib/input/actions/actionTemplates.hpp) |
| Command routing table | [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp) |
| Action factory (route → concrete IAction) | [lib/app/ActionFactory.cpp](../lib/app/ActionFactory.cpp) |
| Concrete command implementations | [lib/app/commands/](../lib/app/commands/) |
| UART toggle/rotary dispatch | [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp) |
| Protocol command IDs | [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp) |

There are **no** global action singleton files (`buttonActions.hpp`/`buttonActions.cpp`). Those have been removed. Actions are created dynamically from the registration table.

## 1. Decide What Kind Of Button You Are Adding

Before editing code, decide which of these paths matches the behavior you want.

### 1.1 Simple UART Command

Use this when one press should produce one command ID and send it to the Raspberry Pi with no extra parameters.

Examples already in the codebase: `k_prevTrack`, `k_nextTrack`, `k_playPause`, `k_nextPanel`.

Registry `ActionSourceType`: `Simple`

### 1.2 Toggle UART Command

Use this when a button alternates between two states and the UART packet needs to carry the current state as a parameter (ON=1 / OFF=0).

Examples already in the codebase: `k_cover`, `k_repeat`, `k_toggleRandom`, `k_toggleMeter`.

Registry `ActionSourceType`: `Toggle` — requires a paired `CMD_*_ON` / `CMD_*_OFF` constant and a row in `ActionUartDispatcher`'s `k_toggleTable[]`.

### 1.3 Timed UART Command

Use this when the button measures how long it was held and sends the hold duration as a parameter (e.g. the power button).

Example: `k_power` with `CMD_SYS_POWER`.

Registry `ActionSourceType`: `Timed`

### 1.4 Local-Only Action

Use this when the button should affect hardware or firmware state on the ESP32 without sending anything to the Raspberry Pi.

Examples already in the codebase: `k_toggleDac` (relay), `k_cycleBrightness` / `k_toggleDisplay` (brightness controller).

These commands are classified away from `UartDispatch` in [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp), and a concrete `IAction` subclass in `lib/app/commands/` performs the side effect.

## 2. Step-By-Step: Add The Command ID First

If the behavior needs a new command ID, define it first in [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp).

Steps:

1. Pick a command name consistent with the current naming scheme, such as `CMD_FOO_BAR`.
2. Pick a unique numeric value that does not collide with existing commands.
3. If the Raspberry Pi is involved, update the Pi-side listener script so it recognizes the new command.
4. Update [docs/protocol-reference.md](./protocol-reference.md) and later [docs/button-command-map.md](./button-command-map.md).

## 3. Step-By-Step: Add Or Reserve A Button ID

If you are adding a brand-new physical button, reserve its ID in [lib/board/boardButtonIds.hpp](../lib/board/boardButtonIds.hpp).

Steps:

1. Add a new constant in `board::buttons`, for example `inline constexpr uint8_t k_mute = 16;`.
2. Increment `k_count` to match the new total.

Notes:

- `ControlBoardButtonIds.hpp` is just a namespace alias (`controlBoardButtons = ::board::buttons`). You do **not** need to edit it — it picks up new constants automatically.
- The SPI LED bitmask uses the button ID as a bit index for non-power buttons, so changing existing indices also shifts LED positions. Reserve new IDs at the end of the list.

## 4. Step-By-Step: Register The Button

All button wiring lives in the `constexpr ButtonRegistration k_buttons[]` table in [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp).

**Add one row — no other file needs to change for paths 1.1 and 1.3** (beyond steps 2 and 3).

```cpp
constexpr ButtonRegistration k_buttons[] = {
    // ... existing rows ...
    { controlBoardButtons::k_mute, ActionSourceType::Simple, CMD_MUTE, CMD_NO_ACTION, LedPolicy::Momentary },
};
```

Column meanings:

| Column | Purpose |
|---|---|
| `buttonId` | Physical button constant from `board::buttons` |
| `type` | `Simple`, `Toggle`, `Timed`, or `Rotary` |
| `cmd1` | Main command (or `CMD_ON` for `Toggle`) |
| `cmd2` | `CMD_OFF` for `Toggle`; use `CMD_NO_ACTION` otherwise |
| `ledPolicy` | `None`, `Momentary`, or `Toggle` |

`LedPolicy` controls the SPI LED feedback:

- `None` — no LED feedback (power button, rotary).
- `Momentary` — LED on while held, off on release.
- `Toggle` — LED state flips on each press.

The registry allocates action sources dynamically from the table, so there are no global singleton instances to declare or define.

## 5. Step-By-Step: Wire The Behavior

This is where the three paths diverge.

### 5.1 Path A: Simple UART Command

No extra work is required beyond steps 2–4. Commands not listed in `k_commandRouteTable[]` default to `ActionCommandRoute::UartDispatch`, and `UartDispatchAction::execute()` sends them as raw UART with no parameters.

### 5.2 Path B: Toggle UART Command

A `Toggle` row in the registry emits either `CMD_*_ON` or `CMD_*_OFF` on alternating presses. The UART layer must collapse these into a single wire command with a boolean parameter.

Add one row to `k_toggleTable[]` in [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp):

```cpp
constexpr ToggleMapping k_toggleTable[] = {
    // ... existing rows ...
    {CMD_MUTE_ON, CMD_MUTE_OFF, CMD_TOGGLE_MUTE, "Mute"},
};
```

The dispatcher will:
1. match `CMD_MUTE_ON` or `CMD_MUTE_OFF`,
2. send a UART packet with `commandId = CMD_TOGGLE_MUTE` and `params[0] = 1` (ON) or `0` (OFF).

### 5.3 Path C: Local-Only Action

Local actions need three additional edits.

**1. Classify the command** — add a row to `k_commandRouteTable[]` in [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp):

```cpp
constexpr CommandRouteEntry k_commandRouteTable[] = {
    // ... existing rows ...
    { CMD_TOGGLE_FAN, ActionCommandRoute::Relay },
};
```

Use an existing route value when the behavior fits an existing concrete class, or add a new `ActionCommandRoute` enum value and a new class if it does not.

**2. Map the route in the factory** — if you added a new `ActionCommandRoute` value, add a case to `createAction()` in [lib/app/ActionFactory.cpp](../lib/app/ActionFactory.cpp):

```cpp
case ActionCommandRoute::Fan:
    return std::make_unique<actions::FanAction>(command);
```

**3. Implement the action** — create `lib/app/commands/FanAction.hpp` and `FanAction.cpp` following the pattern of the existing commands:

- `RelayAction` — [lib/app/commands/RelayAction.cpp](../lib/app/commands/RelayAction.cpp)
- `BrightnessAction` — [lib/app/commands/BrightnessAction.cpp](../lib/app/commands/BrightnessAction.cpp)
- `SystemAction` — [lib/app/commands/SystemAction.cpp](../lib/app/commands/SystemAction.cpp)

The class inherits `actions::IAction` and implements `execute(ActionContext &ctx)` to perform the local side effect.

> Note: `BrightnessAction` handles both `CMD_CYCLE_BRIGHTNESS` and `CMD_TOGGLE_DISPLAY` — both are routed as `ActionCommandRoute::Brightness`. Check whether your new local command fits an existing concrete class before adding a new one.

## 6. Step-By-Step: Update The Raspberry Pi Side When Needed

If the new button sends UART, update the Pi-side listener.

Typical file: [scripts/rpi/home/antho/UAart5Listener.py](../scripts/rpi/home/antho/UAart5Listener.py)

Steps:

1. Add support for the new command ID.
2. If parameters are used, decode them consistently with the firmware encoding.
3. Keep the firmware-side name, numeric ID, and parameter meaning aligned with the protocol reference.

## 7. Step-By-Step: Update The Docs

Once the button works, update the two reference docs.

Files:

- [docs/button-command-map.md](./button-command-map.md)
- [docs/protocol-reference.md](./protocol-reference.md)

## 8. Worked Examples

### 8.1 Example: Add A Simple UART Button (Mute)

Goal: add a `Mute` button that sends `CMD_MUTE` to the Raspberry Pi.

Files to touch:

1. [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp) — add `CMD_MUTE = 0x0120`
2. [lib/board/boardButtonIds.hpp](../lib/board/boardButtonIds.hpp) — add `k_mute = 16`, increment `k_count`
3. [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp) — add one row to `k_buttons[]`
4. Pi-side listener
5. [docs/button-command-map.md](./button-command-map.md) and [docs/protocol-reference.md](./protocol-reference.md)

### 8.2 Example: Add A Toggle UART Button (Mute with state)

Goal: add a `Mute` button that sends `CMD_TOGGLE_MUTE` with `params[0] = 1/0`.

Files to touch:

1. [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp) — add `CMD_MUTE_ON`, `CMD_MUTE_OFF`, `CMD_TOGGLE_MUTE`
2. [lib/board/boardButtonIds.hpp](../lib/board/boardButtonIds.hpp) — add `k_mute`, increment `k_count`
3. [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp) — add `Toggle` row
4. [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp) — add row to `k_toggleTable[]`
5. Pi-side listener
6. Docs

### 8.3 Example: Add A Local Relay Button (Fan)

Goal: add a `Fan` button that flips a relay on the ESP32 and never sends UART.

Files to touch:

1. [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp) — add `CMD_TOGGLE_FAN`
2. [lib/board/boardButtonIds.hpp](../lib/board/boardButtonIds.hpp) — add `k_fan`, increment `k_count`
3. [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp) — add `Simple` row for `CMD_TOGGLE_FAN`
4. [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp) — add `{ CMD_TOGGLE_FAN, ActionCommandRoute::Relay }` (or reuse an existing route)
5. `RelayAction.cpp` — extend `execute()` to handle `CMD_TOGGLE_FAN` if reusing `Relay` route, or create a new command class
6. [docs/button-command-map.md](./button-command-map.md)

## 9. Quick Checklist

Use this checklist when adding a button:

1. Add or confirm the `CMD_*` constant in `uartProtocol.hpp`.
2. Add or confirm the button ID in `boardButtonIds.hpp`; increment `k_count`.
3. Add one row to `k_buttons[]` in `ControlBoardActionRegistry.cpp`.
4. **If toggle UART**: add a row to `k_toggleTable[]` in `ActionUartDispatcher.cpp`.
5. **If local-only**: classify the command in `ActionCommandRoutingPolicy.hpp`, map it in `ActionFactory.cpp`, and implement `execute()`.
6. Update Pi-side handling if UART is involved.
7. Update `button-command-map.md` and `protocol-reference.md`.
8. Run the `PlatformIO Build` task and verify the expected side effect.

## 10. Validation

The normal validation step is the `PlatformIO Build` task from the workspace root.

If the button is UART-backed, also verify:

- toggle commands appear correctly in `k_toggleTable[]` if they carry state,
- the Raspberry Pi side understands the command and parameter encoding.

If the button is local-only, verify:

- the command appears in `k_commandRouteTable[]` with the correct route,
- the correct relay, indicator, or state change actually occurs,
- no accidental UART packet is emitted.