# 09. How To Add A New Button Or Action

This is the practical, hands-on guide for adding a new physical button (or a new
behavior on an existing one) to the ESP32-S3 firmware. It reflects the code as it
exists today — see §7 for how to verify that against a future checkout.

It covers the three generic action shapes the firmware supports, plus the
local-only variant:

| Shape | What it means | Example already in the codebase |
|---|---|---|
| **Momentary / Simple** | One press → one fixed command, no state. LED (if any) is on while held. | `Prev Track`, `Next Track`, `Play/Pause` |
| **Toggle** | Alternates between two states on each press; LED flips each press. | `Repeat`, `Random`, `Cover View`, `Meter` |
| **Timed** | Measures how long the button was held and sends the duration as a parameter. | `Power` button |

## 1. Where Everything Lives

| Concern | File |
|---|---|
| Physical button ID constants | [lib/board/boardButtonIds.hpp](../lib/board/boardButtonIds.hpp) |
| Button ID namespace alias (no edits needed) | [lib/app/ControlBoardButtonIds.hpp](../lib/app/ControlBoardButtonIds.hpp) |
| **Button registration table** (the main thing you edit) | [lib/app/ControlBoardActionRegistry.cpp](../lib/app/ControlBoardActionRegistry.cpp) |
| Action source implementations (Simple/Toggle/Timed/Rotary) | [lib/input/actions/actionTemplates.hpp](../lib/input/actions/actionTemplates.hpp) |
| LED feedback policy (`None`/`Momentary`/`Toggle`) | [lib/app/ControlBoardInputDispatcher.hpp](../lib/app/ControlBoardInputDispatcher.hpp) / `.cpp` |
| UART toggle normalization table | [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp) |
| Local-only command routing | [lib/app/ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp) |
| Route → concrete action mapping | [lib/app/ActionFactory.cpp](../lib/app/ActionFactory.cpp) |
| Concrete local-action implementations | [lib/app/commands/](../lib/app/commands/) |
| Protocol command IDs | [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp) |
| Human-readable command names (logging) | [lib/protocol/commandCatalog.cpp](../lib/protocol/commandCatalog.cpp) |
| Raspberry Pi command handling | `scripts/rpi/home/antho/uart5_listener.py` + `command_ids.py`/`playback_commands.py` |

There are no global action singletons — `ControlBoardActionRegistry::populate()`
constructs one action-source instance per button straight from the table below.

## 2. The Registration Table

Everything starts with one row in `k_buttons[]`
(lib/app/ControlBoardActionRegistry.cpp):

```cpp
constexpr ButtonRegistration k_buttons[] = {
    { controlBoardButtons::k_playPause, ActionSourceType::Simple, CMD_PLAY_PAUSE,     CMD_NO_ACTION,      LedPolicy::Momentary },
    { controlBoardButtons::k_repeat,    ActionSourceType::Toggle, CMD_REPEAT_ON,      CMD_REPEAT_OFF,     LedPolicy::Toggle    },
    { controlBoardButtons::k_power,     ActionSourceType::Timed,  CMD_SYS_POWER,      CMD_NO_ACTION,      LedPolicy::None      },
    // ...
};
```

| Column | Meaning |
|---|---|
| `buttonId` | Physical button constant from `board::buttons` |
| `type` | `ActionSourceType::Simple`, `Toggle`, `Timed`, or `Rotary` |
| `cmd1` | Main command — or `CMD_*_ON` for `Toggle` |
| `cmd2` | `CMD_*_OFF` for `Toggle`; `CMD_NO_ACTION` for everything else |
| `ledPolicy` | `LedPolicy::None`, `Momentary`, or `Toggle` (drives the SPI front-panel LED) |

`LedPolicy` is independent of `ActionSourceType` — e.g. `k_toggleDisplay` is a
`Simple` action source (one fixed command) but still uses `LedPolicy::Toggle`
because the *LED* toggles even though the underlying command doesn't carry state.

## 3. Example 1 — Momentary / Simple Action

**Goal:** add a `Mute` button that sends one fixed command while pressed, LED on
while held.

1. Add the command ID in [lib/protocol/uartProtocol.hpp](../lib/protocol/uartProtocol.hpp):
   ```cpp
   CMD_MUTE = 0x0120,
   ```
2. Reserve a button ID in [lib/board/boardButtonIds.hpp](../lib/board/boardButtonIds.hpp):
   ```cpp
   inline constexpr uint8_t k_mute = 16;
   // ...
   inline constexpr uint8_t k_count = 17;   // bump this
   ```
   Add new IDs at the end — the SPI LED bitmask uses the button ID as a bit index,
   so renumbering existing buttons shifts LED positions on the panel.
3. Add one row to `k_buttons[]`:
   ```cpp
   { controlBoardButtons::k_mute, ActionSourceType::Simple, CMD_MUTE, CMD_NO_ACTION, LedPolicy::Momentary },
   ```
4. Nothing else to wire in firmware: any command not listed in
   `k_commandRouteTable[]` (ActionCommandRoutingPolicy.hpp) defaults to
   `ActionCommandRoute::UartDispatch`, and `UartDispatchAction::execute()` sends it
   as a raw UART command with no parameters.
5. Add a display name to [commandCatalog.cpp](../lib/protocol/commandCatalog.cpp)
   (`{CMD_MUTE, "Mute", "Mute"}`) so logs and the UART command name lookup work.
6. Add handling on the Raspberry Pi side (`command_ids.py` + a handler in
   `uart5_listener.py`'s `COMMAND_HANDLERS`).

What runs under the hood: `SimpleCommandAction::produce(bool isPressed)`
(lib/input/actions/actionTemplates.hpp) returns `nullptr` on release and
`createAction(CMD_MUTE)` on press — one command, no state.

## 4. Example 2 — Toggle Action

**Goal:** add a `Mute` button that alternates ON/OFF and reports its state to the
Pi as a single normalized command with a boolean parameter.

1. Add three command IDs in `uartProtocol.hpp`:
   ```cpp
   CMD_MUTE_ON  = 0x0121,
   CMD_MUTE_OFF = 0x0122,
   CMD_TOGGLE_MUTE = 0x0123,
   ```
2. Reserve the button ID as in §3.
3. Add a `Toggle` row:
   ```cpp
   { controlBoardButtons::k_mute, ActionSourceType::Toggle, CMD_MUTE_ON, CMD_MUTE_OFF, LedPolicy::Toggle },
   ```
4. Add one row to `k_toggleTable[]` in
   [lib/app/ActionUartDispatcher.cpp](../lib/app/ActionUartDispatcher.cpp):
   ```cpp
   {CMD_MUTE_ON, CMD_MUTE_OFF, CMD_TOGGLE_MUTE, CMD_TOGGLE_MUTE, "Mute"},
   ```
   This collapses the alternating `CMD_MUTE_ON`/`CMD_MUTE_OFF` produced by the
   button into one wire command (`CMD_TOGGLE_MUTE`) with `params[0] = 1` (ON) or
   `0` (OFF) — the same normalized command also works if a remote client (BLE/WiFi)
   injects `CMD_TOGGLE_MUTE` directly, since `ActionUartDispatcher::handle()`
   tracks per-row toggle state and flips it whenever the row's `toggleCmd` arrives.
5. Add catalog names for all three IDs.
6. Add Pi-side handling.

What runs under the hood: `DynamicToggleAction::produce(bool isPressed)`
flips an internal `bool m_state` on every press and returns
`createAction(m_state ? cmdOn : cmdOff)`. It also implements
`syncExternalState(bool)` so a remote (BLE/HTTP) toggle keeps the physical
button's next press in sync instead of emitting a stale command.

## 5. Example 3 — Timed Action

**Goal:** add a button whose *hold duration* matters (like `Power`).

1. Add one command ID, e.g. `CMD_FACTORY_RESET_HOLD = 0x0124`.
2. Reserve the button ID.
3. Add a `Timed` row:
   ```cpp
   { controlBoardButtons::k_someButton, ActionSourceType::Timed, CMD_FACTORY_RESET_HOLD, CMD_NO_ACTION, LedPolicy::None },
   ```
4. Decide how the receiving side (a `commands/` class, or the Pi) interprets the
   `releaseMs` parameter — e.g. "hold > 5000ms triggers a factory reset".

What runs under the hood: `DynamicTimedAction::produce(bool isPressed)`
(actionTemplates.hpp) stamps `esp_timer_get_time()` on press and returns
`nullptr`; on release it computes the elapsed microseconds, converts to
milliseconds, and returns `createAction(cmd, releaseMs)`. The duration travels
as the `releaseTimeMillis` field on the produced `IAction` (see
`ActionContext`/`IAction.hpp`) — a UART-dispatched timed command carries it as a
UART parameter, exactly like the existing `CMD_SYS_POWER` handling in
`PowerTransitionAction`.

## 6. Local-Only Actions (No UART)

Some commands should never leave the ESP32 — e.g. flipping a relay or changing
LED brightness. These are still registered as `Simple` (or `Toggle`) in
`k_buttons[]`, but classified away from `UartDispatch`:

1. **Classify it** — add a row to `k_commandRouteTable[]` in
   [ActionCommandRoutingPolicy.hpp](../lib/app/ActionCommandRoutingPolicy.hpp):
   ```cpp
   { CMD_TOGGLE_FAN, ActionCommandRoute::Relay },
   ```
   Reuse an existing `ActionCommandRoute` value (`Relay`, `Brightness`, `System`,
   `PowerStateTransition`) if your new command fits one of the existing concrete
   classes in `lib/app/commands/` — check there before adding a new route.
2. **If you added a brand-new route value**, map it in
   [ActionFactory.cpp](../lib/app/ActionFactory.cpp)'s `createAction()` switch.
3. **Implement the effect** — either extend an existing class's `execute()`
   (e.g. `RelayAction`, `BrightnessAction`, `SystemAction`), or add a new
   `IAction` subclass under `lib/app/commands/` following that pattern
   (`execute(ActionContext&)` + `requiresPowerOn()`).

`requiresPowerOn()` gates the action in `ActionProcessor::process()` — most
local actions return `true` (ignored unless the board is powered on); only
`PowerTransitionAction` returns `false` since it's what turns the board on.

## 7. Verifying This Guide Is Still Accurate

Firmware internals change. Before trusting §2–§6 against a newer checkout,
re-read:

- `lib/app/ControlBoardActionRegistry.cpp` — confirms `ActionSourceType` still
  has exactly `Simple`/`Toggle`/`Timed`/`Rotary`.
- `lib/app/ControlBoardInputDispatcher.hpp` — confirms `LedPolicy` still has
  exactly `None`/`Momentary`/`Toggle`.
- `lib/app/ActionUartDispatcher.cpp` — confirms `k_toggleTable[]` is still the
  UART toggle-normalization mechanism.

## 8. Checklist

1. Add/confirm the `CMD_*` constant(s) in `uartProtocol.hpp`.
2. Add/confirm the button ID in `boardButtonIds.hpp`; bump `k_count`.
3. Add one row to `k_buttons[]` in `ControlBoardActionRegistry.cpp`.
4. Toggle only: add a row to `k_toggleTable[]` in `ActionUartDispatcher.cpp`.
5. Local-only: classify in `ActionCommandRoutingPolicy.hpp`, map in
   `ActionFactory.cpp` if a new route, implement/extend `execute()`.
6. Add display name(s) to `commandCatalog.cpp`.
7. Update the Raspberry Pi listener if UART is involved.
8. Update [button-command-map.md](button-command-map.md).
9. Run the `PlatformIO Build` task and verify on hardware (or via `host_tests`
   for anything with host-testable logic).
