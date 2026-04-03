# Button And Command Map

This document maps the current firmware button indices to action objects and the UART or local effects they produce.

The source of truth for the current mapping is:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
- [lib/controlSystem/ControlBoard.cpp](../lib/controlSystem/ControlBoard.cpp)
- [lib/actions/buttonActions.hpp](../lib/actions/buttonActions.hpp)
- [lib/actions/buttonActions.cpp](../lib/actions/buttonActions.cpp)
- [lib/actions/actionTemplates.hpp](../lib/actions/actionTemplates.hpp)
- [lib/controlSystem/actionProcessor.cpp](../lib/controlSystem/actionProcessor.cpp)

## 1. How To Read This Map

There are four main action patterns in the current code:

- `SimpleCommandAction`: sends one command when the button is pressed.
- `ToggleAction`: flips internal state on press and emits an ON or OFF command.
- `TimedAction`: starts timing on press and emits one command on release with the held duration.
- `RotaryAction`: emits `CMD_ROTARY_ACTION` with parameter `0` for left and `1` for right.

Important behavior detail:

- toggle actions change state on button press only,
- timed actions emit their command on button release,
- simple command actions do nothing on release.

## 2. Button Index Reference

Current button indices from [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp):

| Index | Symbolic Name |
|---|---|
| 0 | `kPower` |
| 1 | `kPrevTrack` |
| 2 | `kNextTrack` |
| 3 | `kSkipForward` |
| 4 | `kSkipBack` |
| 5 | `kPlayPause` |
| 6 | `kStop` |
| 7 | `kCover` |
| 8 | `kNextMenu` |
| 9 | `kMenuSelect` |
| 10 | `kToggleDac` |
| 11 | `kToggleDisplay` |
| 12 | `kToggleMeter` |
| 13 | `kRotaryEventLeft` |
| 14 | `kRotaryEventRight` |
| 15 | `kCycleBrightness` |

## 3. Current Mapping Table

| Index | Name | Action Type | Firmware Output | Final Effect |
|---|---|---|---|---|
| 0 | Power | `TimedAction<CMD_SYS_POWER>` | emits `CMD_SYS_POWER` on release with hold duration | enters ON, SLEEP, or DEEPSLEEP path depending on current state and hold time |
| 1 | Previous Track | simple command | `CMD_PREVIOUS_TRACK` | UART command to Pi |
| 2 | Next Track | simple command | `CMD_NEXT_TRACK` | UART command to Pi |
| 3 | Skip Forward | simple command | `CMD_SKIP_FORWARD` | UART command to Pi |
| 4 | Skip Back | simple command | `CMD_SKIP_BACK` | UART command to Pi |
| 5 | Play/Pause | simple command | `CMD_PLAY_PAUSE` | UART command to Pi |
| 6 | Stop | simple command | `CMD_STOP_TRACK` | UART command to Pi |
| 7 | Cover | `ToggleAction<CMD_COVER_VIEW_ON, CMD_COVER_VIEW_OFF>` | converted to `CMD_TOGGLE_COVER_VIEW` with param `1` or `0` | Pi toggles cover view |
| 8 | Next Menu | simple command | `CMD_NEXT_MENU_ITEM` | UART command to Pi |
| 9 | Menu Select | simple command | `CMD_ITEM_SELECT` | UART command to Pi |
| 10 | Toggle DAC | `ToggleAction<CMD_TOGGLE_DAC_ON, CMD_TOGGLE_DAC_OFF>` | local relay control | toggles DAC power relay |
| 11 | Toggle Display | `ToggleAction<CMD_DISPLAY_OFF, CMD_DISPLAY_ON>` | converted to `CMD_TOGGLE_DISPLAY` with param `0` or `1` | Pi display mode change |
| 12 | Toggle Meter | `ToggleAction<CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF>` | converted to `CMD_TOGGLE_METER` with param `1` or `0` | Pi meter display change |
| 13 | Rotary Left | `RotaryAction<CMD_ROTARY_ACTION>` | `CMD_ROTARY_ACTION` with param `0` | Pi interprets as previous/left |
| 14 | Rotary Right | `RotaryAction<CMD_ROTARY_ACTION>` | `CMD_ROTARY_ACTION` with param `1` | Pi interprets as next/right |
| 15 | Cycle Brightness | simple command | `CMD_CYCLE_BRIGHTNESS` | local brightness cycle |

## 4. Important Per-Button Notes

### 4.1 Power Button

The power button is special.

- press starts a timer,
- release sends `CMD_SYS_POWER`,
- `releaseTimeMillis` decides which power transition runs.

Current threshold:

- short press is less than `3000 ms`,
- long press is `3000 ms` or more.

References:

- [lib/actions/actionTemplates.hpp](../lib/actions/actionTemplates.hpp)
- [lib/controlSystem/actionProcessor.cpp](../lib/controlSystem/actionProcessor.cpp)

### 4.2 Toggle Buttons

Toggle actions keep internal software state in the action object.

That means:

- the first press uses the action's default state,
- later presses alternate between ON and OFF outputs,
- LED retention behavior comes from `keepLedActive` in the response.

Current toggle-backed buttons:

- Cover
- Toggle DAC
- Toggle Display
- Toggle Meter

### 4.3 Display Toggle Specifics

The display toggle is currently defined as:

- ON command type: `CMD_DISPLAY_OFF`
- OFF command type: `CMD_DISPLAY_ON`

Because the toggle state starts as `false`, the first press emits `CMD_DISPLAY_OFF`.

That may be intentional if the display is assumed to start enabled, but it is worth keeping in mind when debugging behavior.

### 4.4 Rotary Events

Both rotary directions use the same action object class and the same command ID.

Direction is carried in parameter 0:

- `0` = left
- `1` = right

## 5. Local Actions Versus UART Actions

### 5.1 Commands That End Up On UART

These actions send a UART command or message to the Raspberry Pi:

- previous track
- next track
- skip forward
- skip back
- play/pause
- stop
- next menu
- menu select
- cover view toggle
- display toggle
- meter toggle
- rotary action
- Raspberry Pi shutdown in power-down paths

### 5.2 Actions That Stay Local On The ESP32

These actions currently stay local:

- power sequencing,
- DAC relay toggle,
- brightness cycling,
- relay shutdown sequencing,
- heartbeat wait and timeout handling.

## 6. Currently Unmapped Or Not Exposed Through A Dedicated Button

Several protocol commands exist but are not currently exposed as separate dedicated button entries in `createButtonActionMap()`, including:

- `CMD_PREV_MENU_ITEM`
- `CMD_EXIT_ITEM`
- `CMD_SYS_NOHEARTBEAT`

This does not mean they are invalid. It only means there is no direct current button assignment for them.

## 7. Related Docs

- [docs/project-guide.md](./project-guide.md)
- [docs/architecture.md](./architecture.md)
- [docs/power-sequencing.md](./power-sequencing.md)
- [docs/protocol-reference.md](./protocol-reference.md)