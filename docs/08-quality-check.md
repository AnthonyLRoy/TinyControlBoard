# 08 — Quality Check

Document version 1.0 — 2026-09-18 — describes commit `402f526` on branch `feature/library-improvements`.

This pass reconciles every factual claim in documents 01–07 against the source files that
were directly inspected during this documentation effort (not just subagent-reported
summaries). Facts that were only available via subagent inventory and not independently
re-read are listed as such below and are additionally marked `TODO` inline in their
respective documents where precision matters.

## Directly Re-Verified Against Source (this session)

| Checked Item | Document / Section | Expected (per brief/prior assumption) | Found (verified in source) | Status |
|---|---|---|---|---|
| Android↔Pi control transport | 01 §5, 02 §9 | Original brief assumed a network/HTTP control path from Android to the Pi | Android controls the system exclusively via BLE to the ESP32; HTTP is used only for album art, direct to the Pi's web server | **Corrected** |
| Relay control path | 03 §12, 06 §6 | Original brief assumed relay control went through an I/O-expander chain | All 7 relays are driven by direct ESP32 GPIO outputs; the MCP23018 expander is input-only (buttons/rotary) | **Corrected** |
| `k_simulateRpiBoot` / `SIMULATE_RPI_BOOT` | 03 (build environments), platformio.ini | Repo memory claimed this flag is "automatically true in debug builds" | `platformio.ini` shows `-DSIMULATE_RPI_BOOT=1` is an explicit build flag on the default (`esp32-s3-devkitc-1-16mb`) environment, not something `boardConfig.hpp` derives implicitly from `NDEBUG`/debug alone | **Corrected** — the debug environment happens to also pass this flag explicitly; both statements describe the same net effect but the mechanism is an explicit build flag, not implicit `#ifdef NDEBUG` logic |
| `lib/wifi/` module existence | Documentation-gaps.md | Repo memory described a WiFi/HTTP command layer (WifiManager, HttpCommandServer, SoftAP) | Confirmed absent from `lib/` at this commit — directory contains only `app/, ble/, board/, hal/, indicators/, input/, power/, protocol/` | **Confirmed removed; documented as BLE-only** |
| UART protocol constants, `CommandId` table, `MessageType` enum, `UartMessage` struct | 05 (entire document) | N/A — first-time documentation | Read verbatim from `lib/protocol/uartProtocol.hpp` in full | **Verified directly, no discrepancy** |
| GPIO/timing/I2C/relay/indicator/BLE constants | 03 §10, 06 (entire document) | N/A — first-time documentation | Read verbatim from `lib/board/boardConfig.hpp` in full | **Verified directly, no discrepancy** |
| Relay active-high polarity | 06 §6 | Assumed active-high (industry convention) | Confirmed in `lib/hal/relay/relay.cpp`: `gpio_set_level(relayPin, state ? 1 : 0)`, pull-down enabled/pull-up disabled | **Confirmed** |
| Firmware boot sequence order | 01 §7, 03 §5 | Subagent-reported sequence | Re-read `src/main.cpp` and `lib/app/ControlBoard.cpp` directly; confirmed order: `esp_pm_configure` → 5000ms delay → `ControlBoard::init()` (NVS → transport → components → relays → MCP → button queue → serial → action registry → `triggerInitialPowerOn`) → BLE server start → idle loop | **Verified directly, no discrepancy** |
| Power-on relay sequence and delays | 01 §7, 03 §12, 06 §2 | Subagent-reported sequence | Re-read `PowerStateTransitionHandler.cpp` directly: VCC3V3 (+1000ms) → DAC (+1500ms) → Output stage (+1500ms) → RPi (+1000ms, then wait ≤60s for heartbeat) | **Verified directly, no discrepancy** |
| Sleep/Deep Sleep relay teardown order | 01 §8, 03 §12 | Subagent-reported sequence | Re-read `PowerStateTransitionHandler.cpp` directly: both run the RPi shutdown sequence first (command, wait ≤60s, relay off, 500ms settle), then Sleep turns off only the output stage, Deep Sleep additionally turns off the DAC relay | **Verified directly, no discrepancy** |
| BLE GATT characteristic UUIDs and property flags | 02 §9, 06 §8 | Subagent-reported table | Re-read `lib/ble/BleServer.cpp` `grep` output directly: 8 characteristics, UUID suffixes `...9d01`–`...9d08`, property flags (`WRITE`/`WRITE_NO_RSP` for command chars, `READ`/`NOTIFY` for status/data chars) | **Verified directly, no discrepancy** |
| Front-panel connector pin 17 / button index 7 mismatch | Documentation-gaps.md, 05 (CMD_PREV_MENU_ITEM row) | Repo memory claim | Confirmed in `docs/wiring-reference.md` §1.7's own mismatch row (`17 | CMD_Prev_Menu_Item | 7 | kCover ... | mismatch`) | **Confirmed, pre-existing and already documented elsewhere; carried into new doc set** |

## Not Independently Re-Verified (carried from subagent inventories with reasonable confidence)

| Item | Document / Section | Reason not re-verified | Marked as |
|---|---|---|---|
| Exact `heartbeat_sender.py` polling intervals (2s/5s/10s) | 04 §8 | Time constraints; only the ESP32-side heartbeat constant (`k_heartbeatTimeoutMs`) was read from primary source | `TODO` inline in 04-rpi-developer-guide.md §8 |
| MCP23018 interrupt-handling task stack/priority | 03 §6 | Not read directly this session | `TODO` inline in the task table |
| `MSG_NACK` dead-code status | 05 §10 | No call-site search performed for this specific enum value | `TODO` inline |
| Exact `uart_config_t` parity/stop-bit fields in `serial.cpp` | 03 §7 | Not read directly this session | `TODO` inline |
| SPI shift-register exact part number | 06 §13 | Not present in any inspected source file | `TODO` inline |
| moOde Audio exact version | 01 §6, 04 §2 | Not present in any inspected source file | `TODO` inline |

## Cross-Document Consistency Check

- `CommandId` values in 02 (Android), 03 (ESP32), 04 (RPi), and 05 (protocol spec) all
  reference the same numeric values sourced from 05's table, which was itself transcribed
  directly from `uartProtocol.hpp` — no divergent values were introduced across documents.
- GPIO pin numbers are identical across 03 §10 and 06 §9 (both sourced from the same
  `boardConfig.hpp` read).
- Timing constants (heartbeat timeout, boot timeout, relay settle delays) are identical
  across 01, 03, and 06 — all trace to the single `board::timing` namespace read once and
  reused.
- The BLE-vs-WiFi architecture correction (see table above) was applied consistently across
  01, 02, 03, and documentation-gaps.md — no document in this set still describes the
  superseded WiFi/HTTP-relay architecture as current.

## Outstanding Items for a Future Pass

All items are already listed with `TODO` markers in their respective documents and
summarized in [documentation-gaps.md](documentation-gaps.md); no invented values were
substituted for any of them.
