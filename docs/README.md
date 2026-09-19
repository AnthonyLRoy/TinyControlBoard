# TinyControlBoard — Documentation Set

## What this system is

TinyControlBoard is a custom front-panel control and power-sequencing system for a
Raspberry-Pi-based audio streamer (moOde Audio). A physical control panel (buttons,
rotary encoder, status LEDs) is wired to an ESP32-S3, which is also responsible for
sequencing the unit's power rails (3.3V, DAC, output stage, Raspberry Pi) and for
relaying user commands to the Raspberry Pi over UART. A Kotlin Android app
("TinyRemote" / "DanStreamer") gives the same control surface remotely over
Bluetooth LE, plus library browsing, search, and playlist management.

## The three software components

| Component | Language / Stack | Responsibility |
|---|---|---|
| **ESP32-S3 firmware** ([lib/](../lib/), [src/](../src/)) | C++17, ESP-IDF (via PlatformIO) | Reads physical buttons/rotary (MCP23018 over I2C), drives front-panel LEDs (SPI shift register + PWM), sequences power relays, talks UART to the Pi, exposes a BLE GATT server to the Android app |
| **Raspberry Pi companion scripts** ([scripts/rpi/](../scripts/rpi/)) | Python 3 (systemd services) | Receives UART commands from the ESP32 and executes them against moOde Audio (MPD protocol, `mpc`/`moodeutl` subprocesses, Chrome DevTools Protocol for UI panel switching); sends heartbeat/now-playing/progress data back to the ESP32 |
| **Android app** ([android/TinyRemote/](../android/TinyRemote/)) | Kotlin, Android Gradle Plugin | Connects directly to the ESP32's BLE GATT server; button grid, playback control, library browse/search, playlist management; fetches album art directly from moOde over HTTP |

## How they communicate

```
Android app  --BLE GATT-->  ESP32-S3  --UART (921600 baud)-->  Raspberry Pi (moOde Audio)
Android app  --HTTP (album art only)------------------------->  Raspberry Pi (moOde web server)
```

The ESP32 and Raspberry Pi are the only components with a wired link (UART5 +
2 GPIO handshake lines). The Android app never talks to the Raspberry Pi over
UART; it only reaches the ESP32 (BLE) and, separately, moOde's own HTTP interface
(cover art / thumbnails only, not control).

See [05-communication-protocol.md](05-communication-protocol.md) for the exact
UART wire format and [02-android-developer-guide.md](02-android-developer-guide.md)
§9 for the BLE GATT contract.

## Where to start

1. New to the project? Read [01-system-architecture.md](01-system-architecture.md) first.
2. Setting up a dev machine? Read [07-development-environment.md](07-development-environment.md).
3. Working on firmware? [03-esp32-developer-guide.md](03-esp32-developer-guide.md).
4. Working on the Raspberry Pi side? [04-rpi-developer-guide.md](04-rpi-developer-guide.md).
5. Working on the Android app? [02-android-developer-guide.md](02-android-developer-guide.md).
6. Changing the wire protocol? [05-communication-protocol.md](05-communication-protocol.md) — read this
   before touching `uartProtocol.hpp`, `protocol.py`, or `BleProtocol.kt`.
7. Working on wiring/GPIO/relays? [06-hardware-interface.md](06-hardware-interface.md).
8. Known gaps, TODOs, and cross-component mismatches: [documentation-gaps.md](documentation-gaps.md).
9. What was checked against source and what was fixed: [08-quality-check.md](08-quality-check.md).
10. Adding a new button or action: [09-adding-button-actions.md](09-adding-button-actions.md)
    (a matching `/add-button-action` prompt lives in `.github/prompts/`).

## Source snapshot

- **Repository**: single monorepo at `d:\Dev\TinyControlBoard` — the Android app, ESP32
  firmware, and Raspberry Pi scripts are all subtrees of this one repository/commit
  (there are no separate Android/RPi/ESP32 repos).
- **Branch**: `feature/library-improvements`
- **Commit**: `402f526` (2026-09-17)
- **Snapshot/inspection date**: 2026-09-18
- Working tree had a handful of locally deleted-but-uncommitted legacy doc files
  (`docs/project-guide.md`, `docs/protocol-reference.md`, `docs/add-button-how-to.md`,
  two old exported `.docx`/`.md` files) at inspection time — not part of this
  documentation set's inputs.

Pre-existing narrower documents (`architecture.md`, `button-command-map.md`,
`power-sequencing.md`, `wiring-reference.md`, `RPI4_SETUP.md`,
`android-tinyremote-user-manual.md`, `wifi-transport-plan.md`,
`stream-dac-front-panel-user-guide.md`) remain in this folder and are referenced
where they contain corroborating detail; this new numbered set supersedes them as
the primary developer-facing reference.
