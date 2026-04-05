# Button And Command Map

This document maps the current firmware button indices to action objects and the UART, SPI, or local effects they produce.

The source of truth for the current mapping is:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
- [lib/controlSystem/ControlBoard.cpp](../lib/controlSystem/ControlBoard.cpp)
- [lib/actions/buttonActions.hpp](../lib/actions/buttonActions.hpp)
- [lib/actions/buttonActions.cpp](../lib/actions/buttonActions.cpp)
- [lib/actions/actionTemplates.hpp](../lib/actions/actionTemplates.hpp)
- [lib/controlSystem/actionProcessor.cpp](../lib/controlSystem/actionProcessor.cpp)
- [lib/indicators/spiLedDriver.cpp](../lib/indicators/spiLedDriver.cpp)

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

SPI LED behavior detail:

- physical button presses call `setLed(buttonId, true)` and set bit `1 << buttonId` in the 16-bit SPI LED register,
- the driver shifts out that full 16-bit register low byte first,
- the value shown in the table below is the per-button bit value contributed by that press, not a guarantee that the full transmitted register will contain only that bit,
- release normally clears the bit again, but toggle actions can leave it latched when `keepLedActive = true`,
- rotary movement currently does not call `setLed()` in `handleRotaryMovement()`, so no SPI LED update is sent for rotary left or rotary right events.

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

The table below separates three things that were previously blended together:

- the physical button index,
- the SPI LED bit pattern written when that button press lights the corresponding LED,
- the command ID and command name produced by the button action.

For toggle-backed buttons, the action can emit one of two raw command IDs depending on current state, and the action processor may then normalize that into a different UART command.

| Index | Button Name | Action Type | SPI Register | SPI Bit Pattern | SPI TX Bytes (low, high) | Raw Command ID(s) | Raw Command Name(s) | Final Routed Command | Final Effect |
|---|---|---|---|---|---|---|---|---|---|
| 0 | Power | `TimedAction<CMD_SYS_POWER>` | `0x0001` | `0000 0000 0000 0001` | `0x01 0x00` | `0x0001` | `CMD_SYS_POWER` | `0x0001 CMD_SYS_POWER` | enters ON, SLEEP, or DEEPSLEEP path depending on current state and hold time |
| 1 | Previous Track | simple command | `0x0002` | `0000 0000 0000 0010` | `0x02 0x00` | `0x0101` | `CMD_PREVIOUS_TRACK` | `0x0101 CMD_PREVIOUS_TRACK` | UART command to Pi |
| 2 | Next Track | simple command | `0x0004` | `0000 0000 0000 0100` | `0x04 0x00` | `0x0100` | `CMD_NEXT_TRACK` | `0x0100 CMD_NEXT_TRACK` | UART command to Pi |
| 3 | Skip Forward | simple command | `0x0008` | `0000 0000 0000 1000` | `0x08 0x00` | `0x0104` | `CMD_SKIP_FORWARD` | `0x0104 CMD_SKIP_FORWARD` | UART command to Pi |
| 4 | Skip Back | simple command | `0x0010` | `0000 0000 0001 0000` | `0x10 0x00` | `0x0105` | `CMD_SKIP_BACK` | `0x0105 CMD_SKIP_BACK` | UART command to Pi |
| 5 | Play/Pause | simple command | `0x0020` | `0000 0000 0010 0000` | `0x20 0x00` | `0x0102` | `CMD_PLAY_PAUSE` | `0x0102 CMD_PLAY_PAUSE` | UART command to Pi |
| 6 | Stop | simple command | `0x0040` | `0000 0000 0100 0000` | `0x40 0x00` | `0x0103` | `CMD_STOP_TRACK` | `0x0103 CMD_STOP_TRACK` | UART command to Pi |
| 7 | Cover | `ToggleAction<CMD_COVER_VIEW_ON, CMD_COVER_VIEW_OFF>` | `0x0080` | `0000 0000 1000 0000` | `0x80 0x00` | `0x0117 / 0x0118` | `CMD_COVER_VIEW_ON / CMD_COVER_VIEW_OFF` | `0x0119 CMD_TOGGLE_COVER_VIEW` with param `1` or `0` | Pi toggles cover view |
| 8 | Next Menu | simple command | `0x0100` | `0000 0001 0000 0000` | `0x00 0x01` | `0x0107` | `CMD_NEXT_MENU_ITEM` | `0x0107 CMD_NEXT_MENU_ITEM` | UART command to Pi |
| 9 | Menu Select | simple command | `0x0200` | `0000 0010 0000 0000` | `0x00 0x02` | `0x0108` | `CMD_ITEM_SELECT` | `0x0108 CMD_ITEM_SELECT` | UART command to Pi |
| 10 | Toggle DAC | `ToggleAction<CMD_TOGGLE_DAC_ON, CMD_TOGGLE_DAC_OFF>` | `0x0400` | `0000 0100 0000 0000` | `0x00 0x04` | `0x010A / 0x010F` | `CMD_TOGGLE_DAC_ON / CMD_TOGGLE_DAC_OFF` | local-only relay toggle, no normalized UART command | toggles DAC power relay |
| 11 | Toggle Display | `ToggleAction<CMD_DISPLAY_OFF, CMD_DISPLAY_ON>` | `0x0800` | `0000 1000 0000 0000` | `0x00 0x08` | `0x010B / 0x010E` | `CMD_DISPLAY_OFF / CMD_DISPLAY_ON` | `0x0114 CMD_TOGGLE_DISPLAY` with param `0` or `1` | Pi display mode change |
| 12 | Toggle Meter | `ToggleAction<CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF>` | `0x1000` | `0001 0000 0000 0000` | `0x00 0x10` | `0x010C / 0x010D` | `CMD_TOGGLE_METER_ON / CMD_TOGGLE_METER_OFF` | `0x0115 CMD_TOGGLE_METER` with param `1` or `0` | Pi meter display change |
| 13 | Rotary Left | `RotaryAction<CMD_ROTARY_ACTION>` | n/a | n/a | n/a | `0x0112` | `CMD_ROTARY_ACTION` | `0x0112 CMD_ROTARY_ACTION` with param `0` | Pi interprets as previous/left |
| 14 | Rotary Right | `RotaryAction<CMD_ROTARY_ACTION>` | n/a | n/a | n/a | `0x0112` | `CMD_ROTARY_ACTION` | `0x0112 CMD_ROTARY_ACTION` with param `1` | Pi interprets as next/right |
| 15 | Cycle Brightness | simple command | `0x8000` | `1000 0000 0000 0000` | `0x00 0x80` | `0x0116` | `CMD_CYCLE_BRIGHTNESS` | `0x0116 CMD_CYCLE_BRIGHTNESS` | local brightness cycle |

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

Unlike the physical push buttons, rotary movement is handled by `handleRotaryMovement()` and does not currently light a button LED through the SPI shift register.

### 4.5 SPI Payload Format For Button LEDs

When a physical button press reaches `handleButtonPressed()`, the firmware calls `indicators::getSpiLedDriver().setLed(buttonPressedId, true)`.

That produces a 16-bit LED register where each button index maps directly to one bit:

- bit 0 = button index 0,
- bit 1 = button index 1,
- ...
- bit 15 = button index 15.

The SPI driver then transmits:

- byte 0 = low 8 bits of the register,
- byte 1 = high 8 bits of the register.

Examples from an idle LED state:

- pressing Power sends register value `0x0001`, which is transmitted as `0x01 0x00`,
- pressing Cover sends register value `0x0080`, which is transmitted as `0x80 0x00`,
- pressing Toggle Display sends register value `0x0800`, which is transmitted as `0x00 0x08`,
- pressing Cycle Brightness sends register value `0x8000`, which is transmitted as `0x00 0x80`.

If another toggle-backed LED is already latched on, the transmitted SPI value will be the OR-combination of the active bits instead of the single-bit value shown in the table.

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