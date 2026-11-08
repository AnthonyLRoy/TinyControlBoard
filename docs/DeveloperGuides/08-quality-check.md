# 08 — Quality Check

Updated 2026-11-08 against the working tree based on commit `de9811a`. The
working tree includes uncommitted firmware, Pi, Android, test and documentation
changes; this record is not a statement about `de9811a` alone or a deployed
device. It summarizes a targeted source and documentation check and the most
recent validation results, not a complete re-audit of every claim in the guides.

The previous version described commit `402f526` on
`feature/library-improvements`. Its historical review is available in Git
history; its shutdown sequence and verification status should not be treated as
current.

## Current Source Checks

| Area | Source checked | Current result |
| --- | --- | --- |
| Architecture | [main.cpp](../../src/main.cpp), [BleServer.cpp](../../lib/ble/BleServer.cpp), [heartbeat_sender.py](../../scripts/rpi/home/antho/heartbeat_sender.py) | Android communicates with the ESP32 over BLE; the Pi exchanges control and playback information with the ESP32 over UART. BLE defines eight characteristics, with UUID suffixes `9d01`–`9d08`. |
| Relay control | [boardConfig.hpp](../../lib/board/boardConfig.hpp), [ControlBoardBootstrap.cpp](../../lib/app/ControlBoardBootstrap.cpp) | Seven relay-control outputs are direct ESP32 GPIOs. The MCP23018 handles input, not relay outputs. Relay initialization failures are logged by the relay driver but are not propagated by `setupRelays()`. Electrical polarity and contacts require hardware confirmation. |
| Startup | [main.cpp](../../src/main.cpp), [ControlBoard.cpp](../../lib/app/ControlBoard.cpp) | After the startup delay, the board initializes its components, relays, MCP, action queue and UART before the initial power-on attempt. MCP callbacks are installed before the attempt but physical input is gated until it ends. An initial Pi boot failure leaves controls available for retry. |
| Power-on | [PowerStateTransitionHandler.cpp](../../lib/power/PowerStateTransitionHandler.cpp), [boardConfig.hpp](../../lib/board/boardConfig.hpp) | Enables 3.3V, DAC, output-stage and Pi relays with 1 s, 1.5 s, 1.5 s and 1 s delays respectively, then waits up to 60 s for a Pi heartbeat. On timeout it switches off the Pi and output-stage relays and enters `SLEEP`. |
| Sleep and deep sleep | [PowerStateTransitionHandler.cpp](../../lib/power/PowerStateTransitionHandler.cpp), [RPIBootManager.cpp](../../lib/power/RPIBootManager.cpp) | Sends a Pi shutdown command, waits up to 60 s for a watchdog timeout, then waits another 5.5 s before cutting Pi power. If the wait fails or recent UART activity is detected after the delay, Pi power is retained and state returns to `ON`. Otherwise, sleep disables the output stage; deep sleep also disables DAC power. |
| Watchdog | [heartbeatWatchdog.hpp](../../lib/hal/uart/heartbeatWatchdog.hpp), [heartbeat_sender.py](../../scripts/rpi/home/antho/heartbeat_sender.py) | The watchdog timeout is 30 s; the Pi sender's nominal intervals are 2 s for progress, 5 s for now-playing and 10 s for heartbeat. Actual received-data time is tracked separately from watchdog rearming. |
| Input and BLE | [ButtonEventQueue.cpp](../../lib/app/ButtonEventQueue.cpp), [BoardBleManager.kt](../../android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ble/BoardBleManager.kt) | Physical inputs and remote action commands share a firmware action queue. Android serializes subscriptions, reads and writes in a GATT queue, with a timeout that closes and attempts to reconnect the connection. |
| UART and library commands | [uartReceiver.cpp](../../lib/hal/uart/uartReceiver.cpp), [BleServer.cpp](../../lib/ble/BleServer.cpp) | The receiver discards oversized payload headers and resynchronizes. BLE library commands accept a second 16-bit parameter when present; four-byte commands remain accepted. |
| Pi playlist load | [playlist_manager.py](../../scripts/rpi/home/antho/playlist_manager.py), [mpd_client.py](../../scripts/rpi/home/antho/mpd_client.py) | Loads the requested playlist before stopping playback and removing the old queue range. MPD errors and premature connection closure are reported rather than treated as success. The sequence is not atomic against other MPD clients. |
| Playlist names | [BleProtocol.kt](../../android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ble/BleProtocol.kt), [PlaylistManagementActivity.kt](../../android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/PlaylistManagementActivity.kt) | The overwrite decision waits for a complete listing or the empty-list indication. |

**Safety limitation:** A UART timeout indicates silence, not proof that the Pi
operating system has halted. A broken UART link can also cause silence. The
grace-period activity check does not eliminate this risk. Do not treat the
shutdown behavior as hardware-verified; see
[Hardware Interface](06-hardware-interface.md).

## Validation Record

The following results were obtained in the preceding implementation session on
this working tree. They are not new runs performed during this documentation
update.

| Check | Result | Limits |
| --- | --- | --- |
| PlatformIO `esp32-s3-devkitc-1-16mb-release` build | Passed | Reported a flash-size mismatch; see below. Build success does not establish relay or hardware behavior. |
| Android `:app:testDebugUnitTest` with Gradle offline | Passed | Includes listing-completion tests; does not validate GATT operations on a device. |
| `python -m unittest discover -s test -p 'test_rpi_review.py'` | 7 passed | Mocked WebSocket, MPD and playlist tests; not an integration test against a running Pi or MPD. |
| `python behavior_test.py` | 8 assertions passed | Mocked behavior checks. On Windows the prior run used `PYTHONIOENCODING=utf-8`. |
| `git diff --check` | Passed before this documentation update | Checks whitespace only. |
| Host C++ tests | Not run | CMake configuration could not find a native compiler/build tool. The added UART and watchdog tests therefore remain unexecuted. |
| Hardware / end-to-end | Not run | Relay timing, boot-failure retry, MCP teardown, BLE reconnect/queue behavior, and move-track forwarding need device tests. |

## Open Discrepancies and Follow-up

1. **Flash and partition configuration:** [platformio.ini](../../platformio.ini)
   selects the 16 MB board and specifies `partitions.csv`, but
   [release sdkconfig](../../sdkconfig.esp32-s3-devkitc-1-16mb-release) selects
   2 MB and the single-app partition table. The release build reported the
   flash-size mismatch. Reconcile the effective build configuration and confirm
   the physical flash capacity and partition layout before flashing.
2. **Boot simulation flag:** The default PlatformIO environment passes
   `SIMULATE_RPI_BOOT=1`, but [boardConfig.hpp](../../lib/board/boardConfig.hpp)
   unconditionally defines it as `0` before evaluating it. The prior quality
   check's claim that the build flag alone determines the setting is therefore
   not reliable. Reconcile the definition, build flags and guide before relying
   on simulation behavior.
3. **Pi guide inventory:** [04-rpi-developer-guide.md](04-rpi-developer-guide.md)
   lists `library_commands.py`, `playlist_commands.py`, `panel_commands.py`,
   `power_commands.py` and service files that do not match the current
   repository inventory. The current modules include
   [library_browser.py](../../scripts/rpi/home/antho/library_browser.py),
   [playlist_manager.py](../../scripts/rpi/home/antho/playlist_manager.py) and
   [panel_control.py](../../scripts/rpi/home/antho/panel_control.py). Verify
   deployed service names on the Pi before updating installation instructions.
4. **moOde version:** The Pi guide states `10.3.2`, but this check did not
   establish the installed version. Confirm it on the Pi rather than assuming
   the documented value applies.
5. **Wiring reference:** The previous review referred to a
   `docs/wiring-reference.md` connector mismatch; that path is absent from the
   current tree. Locate the authoritative wiring drawing and verify the
   connector mapping on hardware before resolving that historical item.
6. **Playlist concurrency:** A separate MPD client can change the queue between
   the length query, load and deletion. The current load protection does not
   make the replacement transaction atomic.
7. **Remaining validation:** Run host tests with a suitable native compiler and
   perform the hardware checks above, including shutdown timeout, resumed UART
   traffic, relay order and timing, and Android disconnect/reconnect cases.

For command-ID ownership and the participant-specific update checklist, see
[Adding Button Actions](09-adding-button-actions.md) and the
[Communication Protocol](05-communication-protocol.md). Cross-language IDs,
hardware wiring and deployed software should be checked when they change;
this record does not certify every value across those surfaces.
