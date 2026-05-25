# Button And Command Map

This document maps the current firmware button indices to action objects and the UART, SPI, or local effects they produce.

The source of truth for the current mapping is:

- [lib/board/boardConfig.hpp](../lib/board/boardConfig.hpp)
- [lib/app/ControlBoard.cpp](../lib/app/ControlBoard.cpp)
- [lib/input/actions/buttonActions.hpp](../lib/input/actions/buttonActions.hpp)
- [lib/input/actions/buttonActions.cpp](../lib/input/actions/buttonActions.cpp)
- [lib/input/actions/actionTemplates.hpp](../lib/input/actions/actionTemplates.hpp)
- [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp)
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

- the SPI LED driver accepts a zero-based `ledIndex` (0–15) and knows nothing about button IDs,
- the caller (`ControlBoard`) now passes the button ID directly as the LED index for non-power buttons: `ledIndex = buttonId`,
- button 0 (power) has no corresponding LED and is skipped — `setLed()` is not called for it,
- because button 0 is skipped, SPI LED bit 0 is currently unused in the normal button path,
- for all other buttons, the driver sets bit `1 << ledIndex` in the 16-bit SPI LED register,
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
| 8 | `kRepeat` |
| 9 | `kToggleRandom` |
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
| 0 | Power | `TimedAction<CMD_SYS_POWER>` | n/a | n/a | n/a | `0x0001` | `CMD_SYS_POWER` | `0x0001 CMD_SYS_POWER` | enters ON, SLEEP, or DEEPSLEEP path depending on current state and hold time; no SPI LED |
| 1 | Previous Track | simple command | `0x0002` | `0000 0000 0000 0010` | `0x02 0x00` | `0x0101` | `CMD_PREVIOUS_TRACK` | `0x0101 CMD_PREVIOUS_TRACK` | UART command to Pi |
| 2 | Next Track | simple command | `0x0004` | `0000 0000 0000 0100` | `0x04 0x00` | `0x0100` | `CMD_NEXT_TRACK` | `0x0100 CMD_NEXT_TRACK` | UART command to Pi |
| 3 | Skip Forward | simple command | `0x0008` | `0000 0000 0000 1000` | `0x08 0x00` | `0x0104` | `CMD_SKIP_FORWARD` | `0x0104 CMD_SKIP_FORWARD` | UART command to Pi |
| 4 | Skip Back | simple command | `0x0010` | `0000 0000 0001 0000` | `0x10 0x00` | `0x0105` | `CMD_SKIP_BACK` | `0x0105 CMD_SKIP_BACK` | UART command to Pi |
| 5 | Play/Pause | simple command | `0x0020` | `0000 0000 0010 0000` | `0x20 0x00` | `0x0102` | `CMD_PLAY_PAUSE` | `0x0102 CMD_PLAY_PAUSE` | UART command to Pi |
| 6 | Stop | simple command | `0x0040` | `0000 0000 0100 0000` | `0x40 0x00` | `0x0103` | `CMD_STOP_TRACK` | `0x0103 CMD_STOP_TRACK` | UART command to Pi |
| 7 | Cover | `ToggleAction<CMD_COVER_VIEW_ON, CMD_COVER_VIEW_OFF>` | `0x0080` | `0000 0000 1000 0000` | `0x80 0x00` | `0x0117 / 0x0118` | `CMD_COVER_VIEW_ON / CMD_COVER_VIEW_OFF` | `0x0119 CMD_TOGGLE_COVER_VIEW` with param `1` or `0` | Pi toggles cover view |
| 8 | Repeat | `ToggleAction<CMD_REPEAT_ON, CMD_REPEAT_OFF>` | `0x0100` | `0000 0001 0000 0000` | `0x00 0x01` | `0x011A / 0x011B` | `CMD_REPEAT_ON / CMD_REPEAT_OFF` | `0x011C CMD_TOGGLE_REPEAT` with param `1` or `0` | Pi toggles repeat mode |
| 9 | Toggle Random | `ToggleAction<CMD_RANDOM_ON, CMD_RANDOM_OFF>` | `0x0200` | `0000 0010 0000 0000` | `0x00 0x02` | `0x011D / 0x011E` | `CMD_RANDOM_ON / CMD_RANDOM_OFF` | `0x011F CMD_TOGGLE_RANDOM` with param `1` or `0` | Pi toggles random mode |
| 10 | Toggle DAC | `ToggleAction<CMD_TOGGLE_DAC_ON, CMD_TOGGLE_DAC_OFF>` | `0x0400` | `0000 0100 0000 0000` | `0x00 0x04` | `0x010A / 0x010F` | `CMD_TOGGLE_DAC_ON / CMD_TOGGLE_DAC_OFF` | local-only relay toggle, no normalized UART command | toggles DAC power relay |
| 11 | Next Panel | simple command | `0x0800` | `0000 1000 0000 0000` | `0x00 0x08` | `0x0107` | `CMD_NEXT_MENU_ITEM` | `0x0107 CMD_NEXT_MENU_ITEM` | UART command to Pi; Pi cycles to next moOde panel |
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

- [lib/input/actions/actionTemplates.hpp](../lib/input/actions/actionTemplates.hpp)
- [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp)

### 4.2 Toggle Buttons

Toggle actions keep internal software state in the action object.

That means:

- the first press uses the action's default state,
- later presses alternate between ON and OFF outputs,
- LED retention behavior comes from `keepLedActive` in the response.

Current toggle-backed buttons:

- Cover
- Toggle DAC
- Toggle Meter
- Repeat
- Toggle Random

### 4.3 Rotary Events

Both rotary directions use the same action object class and the same command ID.

Direction is carried in parameter 0:

- `0` = left
- `1` = right

Unlike the physical push buttons, rotary movement is handled by `handleRotaryMovement()` and does not currently light a button LED through the SPI shift register.

### 4.4 SPI Payload Format For Button LEDs

When a physical button press reaches `handleButtonPressed()`, the firmware uses the button ID directly as the LED index and calls `indicators::getSpiLedDriver().setLed(buttonId, true)`.

Button 0 (power) is skipped — it has no corresponding LED. For all other buttons, the LED index is `buttonId`, which produces a 16-bit LED register where:

- bit 0 is currently unused in the normal button path,
- bit 1 = button index 1 (Previous Track),
- bit 2 = button index 2 (Next Track),
- ...
- bit 15 = button index 15 (Cycle Brightness).

The SPI driver (`SpiLedDriver`) accepts a raw `ledIndex` (0–15) and sets bit `1 << ledIndex`. It has no knowledge of button IDs — the mapping is entirely in `ControlBoard`.

The SPI driver then transmits:

- byte 0 = low 8 bits of the register,
- byte 1 = high 8 bits of the register.

Examples from an idle LED state:

- pressing Previous Track (button 1) sets LED index 1, register value `0x0002`, transmitted as `0x02 0x00`,
- pressing Cover (button 7) sets LED index 7, register value `0x0080`, transmitted as `0x80 0x00`,
- pressing Toggle Display (button 11) sets LED index 11, register value `0x0800`, transmitted as `0x00 0x08`,
- pressing Cycle Brightness (button 15) sets LED index 15, register value `0x8000`, transmitted as `0x00 0x80`.

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
- panel cycling (next panel)
- cover view toggle
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