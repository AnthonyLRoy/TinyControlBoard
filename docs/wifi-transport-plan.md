# Plan: Add WiFi as a second transport alongside BLE

## Decisions (confirmed with user)
- Dual transport: BLE stays, WiFi added as a second, manually-selected transport (not a replacement).
- Status updates: WebSocket push from board (replaces the old 500ms-HTTP-polling design from the archived plan).
- Home-network discovery: try mDNS first (time-boxed spike), fall back to UDP broadcast beacon (proven working previously).
- Provisioning: keep SoftAP + in-app provisioning screen (board hosts "TinyControlBoard-XXXXXX" AP w/ password `TcbSetup!` until it has home WiFi creds).
- Transport choice UI: manual toggle/picker screen, remembered across launches.
- Network routing: the Android WiFi transport binds its HTTP, WebSocket, and discovery traffic to the board WiFi `Network`; opening WiFi settings is only a pre-Android-10 fallback.
- LAN authorization: the initial release assumes a trusted home LAN but requires a board-specific pairing token for command, status, WebSocket, and all post-setup provisioning requests. The one-time initial SoftAP setup request is protected by WPA2 and issues the token. This prevents accidental or unauthorized local clients, but plain HTTP does not protect against an active LAN attacker; add TLS later if that threat model matters.

## Key existing facts (verified in current code, 2026-07-26)
- Current codebase is 100% BLE (`lib/ble/BleServer.{hpp,cpp}`, Android `com.tinycb.remote.ble.*`). No WiFi code remains — it was fully removed when the project pivoted from the original WiFi plan to BLE (see repo memory `wifi-remote-android-plan.md`, archived section).
- Firmware hooks WiFi needs already exist and are mostly reused as-is:
  - `ActionProcessor::injectCommand(CommandId, uint16_t releaseMs=0)` — [lib/app/actionProcessor.cpp](../lib/app/actionProcessor.cpp) — already thread-safe for concurrent external callers via an atomic best-effort busy flag, so BLE and WiFi tasks can both call it concurrently. It currently returns `void` when it drops a busy command; change it to return acceptance so HTTP clients never receive a false success.
  - `ControlBoard::getActionProcessor()` / `getSystemState()` — [lib/app/ControlBoard.hpp](../lib/app/ControlBoard.hpp) lines 27-28.
  - `SystemState::powerState` (atomic enum) + `buttonLedBitmask` (atomic uint16_t) — same 3-byte status shape BLE already uses: `[powerState_u8, bitmask_lo, bitmask_hi]` ([lib/ble/BleServer.cpp](../lib/ble/BleServer.cpp) `pushStatusNotification()`/`statusChrAccess()`).
- `commandCatalog` enumeration helpers (getCommandCatalogCount/getCommandAtIndex/getCommandIdByName) from the OLD wifi plan were removed — current [lib/protocol/commandCatalog.hpp](../lib/protocol/commandCatalog.hpp) only has `getCommandNameById`/`getSimpleCommandLogTag`. Both BLE and the new WiFi transport should just use numeric CommandId like BLE does (Android already hardcodes `ButtonCatalog.kt` with commandIds) — do NOT resurrect the catalog-enumeration REST endpoint, it's unnecessary scope.
- sdkconfig gaps to fill: `CONFIG_ESP_WIFI_ENABLED` is only `y` in `sdkconfig.esp32-s3-devkitc-1-16mb` (leftover), NOT set in `sdkconfig.base` or the release env. `CONFIG_HTTPD_WS_SUPPORT` is unset everywhere. `CONFIG_LWIP_MAX_SOCKETS=10` already present (known needed: set `lru_purge_enable=true` on the httpd config, per a previously-fixed ENFILE bug documented in repo memory).
- [src/CMakeLists.txt](../src/CMakeLists.txt) currently only `REQUIRES bt nvs_flash`; will need `esp_http_server esp_wifi esp_netif esp_event lwip json` added too.
- **mDNS blocker CONFIRMED STILL PRESENT** (re-checked 2026-07-26): `C:\Users\antho\.platformio\packages\framework-espidf\components\mdns` does not exist locally — mdns is registry-only for this ESP-IDF/PlatformIO combo, and repo memory documents `idf-component-manager` is pinned to v1.2.3 which can't parse the current mdns manifest (`Unknown keys: repository_info`), with the pin silently restored on every build. Treat the mDNS attempt as a short, timeboxed spike (~30-60 min) expected to fail; don't over-invest before falling back to the UDP beacon (which is a known-working, zero-dependency design from the archived plan).
- Android app currently has NO networking dependencies at all (no Retrofit/OkHttp/Gson — they were dropped when BLE replaced WiFi). Will need to add OkHttp (handles both plain HTTP calls and WebSocket client in one lib, no need for Retrofit/Gson given the tiny endpoint surface).
- AndroidManifest.xml has no `INTERNET`/WiFi permissions at all currently (BLE-only permissions). `ScanActivity` is currently the LAUNCHER activity.
- No leftover WiFi Android files exist (DiscoveryListener/ApiClient/WifiConnectionHelper etc. were never rebuilt) — clean slate on Kotlin side.

## Steps

### Protocol and authorization contract

Define this contract before firmware and Android work proceed in parallel:

- `POST /api/command` accepts `{"id": <integer>, "releaseMs": <integer, optional>}`. It validates the JSON shape, command ID, release duration, and request-body size before attempting injection.
- `GET /api/status` and WebSocket status frames use `{"version": 1, "powerState": "ON", "buttonLedBitmask": 128}`. The `powerState` string mapping is canonical and covers every `ControlBoardPowerState` value.
- `GET /ws` immediately sends a status frame after a successful authenticated upgrade, then sends only changed status values. The app reconnects with bounded exponential backoff while the WiFi panel is foreground.
- All normal endpoints require `X-TCB-Token`, including the WebSocket upgrade request. During initial SoftAP setup only, permit `POST /provision` without a token because WPA2 access to that setup network is the bootstrap credential; generate a cryptographically random token, return it in that provisioning response, and store it in Android private preferences. Once a token exists, provisioning also requires it. A factory reset clears WiFi credentials and the token.
- Return defined outcomes: `400` malformed/oversized request, `401` missing or invalid token, `404` unknown endpoint, `422` invalid command or parameters, `503` action processor busy, and `500` internal failure. A command returns success only when the processor accepted it.
- This token is deliberate defense for a trusted home LAN, not confidentiality: plain HTTP leaves it visible to an active same-LAN attacker. Treat TLS as a separate future security project if that is in scope.

### Phase A — Firmware: WiFi transport module (parallel with Phase C Android work once API shape below is fixed)
1. Add `lib/wifi/WifiCredentials.{hpp,cpp}` — NVS-backed ssid/pass and pairing token using `support::NvsStorage` from `lib/hal/storage/nvsStorage.hpp`. Use namespace `"wifi_cfg"`, keys `"ssid"`/`"pass"`/`"token"`; `clear()` via `nvs_erase_key` also removes the token during a factory reset.
2. Add `lib/wifi/WifiManager.{hpp,cpp}` — `start(ActionProcessor&, SystemState&)`: init netif/event loop once, STA connect w/ NVS creds (15s timeout via `EventGroupHandle_t`), fallback to WPA2 SoftAP `TinyControlBoard-XXXXXX` (last 3 MAC bytes) w/ password `board::wifi::k_softApPasswordDefault` ("TcbSetup!"), fixed IP `192.168.4.1` in AP mode. Starts `HttpCommandServer` regardless of outcome.
3. Add `lib/wifi/HttpCommandServer.{hpp,cpp}` — simplified vs. the archived design (no catalog endpoints):
  - `POST /api/command` → validate/authenticate body, call the result-returning `injectCommand()`, and return the protocol-defined status code.
  - `GET /api/status` → authenticated initial status fetch using the versioned JSON contract.
  - `GET /ws` → authenticated WebSocket upgrade (`CONFIG_HTTPD_WS_SUPPORT=y`); manage a small fixed client set (e.g. four clients), remove failed/disconnected clients, send an initial frame after the upgrade, and serialize sends through the HTTP server's async API.
  - A FreeRTOS status task polls `SystemState` every ~200 ms and `httpd_ws_send_frame_async`s a versioned JSON frame to all live clients only when state changes, mirroring BLE's change-detection behavior.
  - `POST /provision` → validate body, permit the one-time tokenless request only while in initial SoftAP setup and no token exists, otherwise authenticate it; save credentials, return the newly generated pairing token during initial setup, then `esp_restart()` after a short delay task so the HTTP response flushes first.
  - `DELETE /provision` → authenticated factory reset: clear credentials and token.
   - Set `config.lru_purge_enable = true` on the httpd config (known fix for socket-exhaustion, previously hit and fixed — see repo memory `wifi-remote-android-plan.md`).
4. Change `ActionProcessor::injectCommand()` to report whether the command was accepted. BLE may ignore the return value; `HttpCommandServer` maps a busy result to `503` rather than claiming success.
5. `lib/board/boardConfig.hpp`: add `namespace wifi` constants — `k_softApPasswordDefault`, `k_staConnectTimeoutMs` (15000), `k_deviceName` ("TinyControlBoard"), `k_discoveryUdpPort` (47269), `k_discoveryBeaconIntervalMs` (2000), `k_mdnsHostname` ("tinycontrolboard"), and bounded HTTP/WS client/request sizes.
6. `src/CMakeLists.txt`: add `lib/wifi/*.cpp` to `GLOB_RECURSE`; extend `REQUIRES` to `bt nvs_flash esp_http_server esp_wifi esp_netif esp_event lwip json`.
7. sdkconfig: set `CONFIG_ESP_WIFI_ENABLED=y` and `CONFIG_HTTPD_WS_SUPPORT=y` consistently in `sdkconfig.base`, `sdkconfig.esp32-s3-devkitc-1-16mb`, and `sdkconfig.esp32-s3-devkitc-1-16mb-release` (all three currently kept in sync for `CONFIG_BT_ENABLED`, follow that pattern). Verify BLE+WiFi coexistence doesn't need extra `esp_coex` config (ESP32-S3 shares one radio; ESP-IDF handles time-division coexistence automatically when both `CONFIG_BT_ENABLED` and `CONFIG_ESP_WIFI_ENABLED` are set — just confirm via build + runtime test, no code changes expected).
8. `src/main.cpp`: add `static wifi::WifiManager wifiManager; wifiManager.start(board.getActionProcessor(), board.getSystemState());` right after the existing `bleServer.start(...)` call. *(depends on 1-7)*

### Phase B — Firmware: home-network discovery
9. **mDNS spike** (timeboxed): attempt adding `espressif/mdns` via `idf_component.yml` on `lib/wifi` (or project-root) exactly as before; expect the same `Invalid manifest format` failure given the confirmed-absent local component and documented component-manager pin. If it succeeds, publish one explicit discovery contract: host `tinycontrolboard.local`, DNS-SD service `_tinycontrolboard._tcp`, port `80`. If it fails within the timebox, abandon it — do not manually vendor mDNS sources without checking back with the user.
10. **UDP broadcast beacon fallback** (the dependable implementation): `WifiManager::startDiscoveryBeacon()` — FreeRTOS task broadcasting `{"version":1,"device":"TinyControlBoard","ip":"<ip>","port":80}` via plain lwIP UDP sockets to `255.255.255.255:47269` every 2s, only when in STA mode (not needed in AP mode, which has a fixed known IP).

### Phase C — Android: WiFi transport layer (parallel with Phase A/B once endpoint/JSON shapes above are fixed; needs OkHttp dependency added first)
11. `gradle/libs.versions.toml` + `app/build.gradle.kts`: add OkHttp (`com.squareup.okhttp3:okhttp`) — covers authenticated plain HTTP calls and the native WebSocket client, no Retrofit/Gson needed given the small endpoint surface.
12. Define a neutral `BoardTransport` interface for `MainActivity`: `connectionState`, `status`, `sendCommand()`, and `disconnect()`. Keep `BluetoothDevice`, scan results, and BLE-specific connection states in the BLE package rather than moving the current Bluetooth-shaped `ConnectionState` unchanged. Give WiFi its own connection/provisioning states.
13. New package `com.tinycb.remote.wifi`:
  - `BoardWifiManager.kt` — OkHttp-based; `connect(network, baseUrl, token)` binds the client socket factory to the supplied Android `Network`, opens an authenticated WS to `ws://<host>/ws`, performs an initial `GET /api/status`, and updates the transport-neutral state/status flows. It retries the WS with bounded exponential backoff only while the WiFi panel is foreground; `sendCommand(id)` uses authenticated `POST /api/command`; `disconnect()` closes the WS and releases the bound network.
    - `BoardWifiManagerHolder.kt` — app-wide singleton, same double-checked-locking pattern as `BoardBleManagerHolder.kt` (same Activity-scoped-ViewModel pitfall applies — reuse the lesson already recorded in repo memory).
  - `WifiConnectionHelper.kt` — on Android 10+, request the board SoftAP through `WifiNetworkSpecifier` and expose the resulting `Network` to `BoardWifiManager`; request/track the needed runtime permissions. On older Android versions, open WiFi settings as a fallback, then validate reachability before continuing. SSID display is advisory only, not a routing guarantee.
  - `DiscoveryListener.kt` — if the timeboxed mDNS spike succeeded, use either DNS-SD browse/resolve for `_tinycontrolboard._tcp` or hostname resolution for `tinycontrolboard.local`, according to the chosen firmware contract; do not mix the two. Otherwise, listen for the versioned UDP beacon on port 47269 using the board-bound `Network` and expose the address via a `StateFlow`/callback.
14. `res/xml/network_security_config.xml` — permit cleartext HTTP for `192.168.4.1` (AP mode) and the general home LAN (Android security-config cannot express CIDR ranges, so base-config cleartext remains app-wide); reference via `android:networkSecurityConfig` in the manifest `<application>` tag.
15. `AndroidManifest.xml`: add `INTERNET`, `ACCESS_WIFI_STATE`, `ACCESS_NETWORK_STATE`, `CHANGE_WIFI_STATE`, `NEARBY_WIFI_DEVICES` (API 33+), and any required location permission for the supported Android versions. Add matching runtime permission flows, register new activities, and wire `network_security_config.xml`.

### Phase D — Android: transport selection UI (*depends on 11, and needs 12 mostly done for wiring*)
16. New activities:
    - `ui/TransportPickerActivity.kt` — becomes the new LAUNCHER activity (replaces `ScanActivity`); two buttons "Bluetooth" / "WiFi", persists choice via a new `transport/TransportPrefs.kt` (SharedPreferences), navigates to `ScanActivity` (BLE, unchanged) or `ui/WifiConnectActivity.kt` (new).
    - `ui/WifiConnectActivity.kt` — requests/binds the board SoftAP through `WifiConnectionHelper`, presents WiFi settings only for the legacy fallback, and auto-navigates to `MainActivity` once `BoardWifiManager` reaches `Connected`. It handles reachability, permission, binding, connecting, and reconnecting states distinctly.
    - `ui/WifiProvisioningActivity.kt` — SSID + password (masked) EditTexts, authenticated `POST http://192.168.4.1/provision`, receives/stores the pairing token, shows a restart state, then navigates back to `WifiConnectActivity` to wait for the board to rejoin and be discovered.
    - Add a "Change transport" overflow-menu item in `MainActivity` → back to `TransportPickerActivity`.
17. `viewmodel/MainViewModel.kt`: accept the active `TransportMode` (read from `TransportPrefs`), expose a `BoardTransport` implementation for the selected mode, and keep `MainActivity` limited to transport-neutral state/status/command operations. BLE scan/connect remains owned by `ScanActivity`; WiFi setup/connect remains owned by `WifiConnectActivity`.
18. `AndroidManifest.xml`: launcher intent-filter moves from `ScanActivity` to `TransportPickerActivity`; `ScanActivity` keeps its own intent-filter removed (becomes a normal exported/non-exported activity like `MainActivity`).

### Phase E — Verification (depends on all prior phases)
1. `PlatformIO Build` task — confirm SUCCESS with both BT + WiFi enabled, check RAM/Flash deltas vs BLE-only baseline (RAM 7.8%/Flash 3.7%).
2. Flash firmware; confirm SoftAP `TinyControlBoard-XXXXXX` still advertises AND BLE still advertises simultaneously (coexistence check).
3. Use authenticated curl requests against `192.168.4.1`: validate status, valid command acceptance, malformed JSON (`400`), invalid command (`422`), missing token (`401`), and a busy processor (`503`) without reporting a false command success.
4. Use an authenticated WS test client (e.g. `websocat` with `X-TCB-Token`) → confirm the initial versioned status frame and push-on-change frames arrive within ~200ms of a button press.
6. Provision to home WiFi via the app or curl `/provision`; confirm board reboots, rejoins home network, and either mDNS resolves `tinycontrolboard.local` or the UDP beacon is received (whichever path wins).
7. Android: `assembleDebug` build; install; run through `TransportPickerActivity` → WiFi path end-to-end (SoftAP network binding, provisioning/token storage, discovery, button press, status LED update), verify Android 10+ traffic stays on the board WiFi despite cellular availability, and confirm the BLE path (`ScanActivity`) still works unchanged.
8. host_tests (34/34) — re-run to confirm no regressions in the pure-logic layer (WiFi/BLE additions are both firmware-transport-layer only, shouldn't touch host-testable code, but verify).

## Relevant files
- `lib/app/actionProcessor.{hpp,cpp}` — small API change so injection acceptance reaches the HTTP response; `lib/app/ControlBoard.hpp` and `lib/app/SystemState.hpp` otherwise reused as-is.
- `lib/ble/BleServer.{hpp,cpp}` — reference implementation for the status-push change-detection pattern and injectCommand call site; unchanged.
- `lib/protocol/commandCatalog.hpp` — confirm no catalog-enumeration endpoint needed; unchanged.
- NEW: `lib/wifi/WifiCredentials.{hpp,cpp}`, `lib/wifi/WifiManager.{hpp,cpp}`, `lib/wifi/HttpCommandServer.{hpp,cpp}`
- `lib/board/boardConfig.hpp`, `src/CMakeLists.txt`, `src/main.cpp`, `sdkconfig.base`, `sdkconfig.esp32-s3-devkitc-1-16mb`, `sdkconfig.esp32-s3-devkitc-1-16mb-release`
- `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ble/*` — BLE scanning and Bluetooth-specific states stay here.
- NEW: a transport-neutral `BoardTransport` abstraction and WiFi-specific connection/provisioning state types.
- NEW: `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/wifi/{BoardWifiManager,BoardWifiManagerHolder,WifiConnectionHelper,DiscoveryListener}.kt`
- NEW: `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/transport/{TransportMode,TransportPrefs}.kt`
- NEW: `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/{TransportPickerActivity,WifiConnectActivity,WifiProvisioningActivity}.kt` + matching layouts
- `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/viewmodel/MainViewModel.kt`, `.../ui/MainActivity.kt` (menu item only), `AndroidManifest.xml`, `app/build.gradle.kts`, `gradle/libs.versions.toml`
- NEW: `android/TinyRemote/app/src/main/res/xml/network_security_config.xml`

## Scope boundaries
- IN: dual BLE+WiFi transport, manual picker, authenticated HTTP/WebSocket status push, SoftAP+provisioning, Android board-network binding, mDNS-spike-then-UDP-beacon discovery, and defined command/error behavior.
- OUT (unless requested later): command-catalog REST enumeration endpoint, TLS/HTTPS, Play Store packaging, auto-fallback between transports (user explicitly chose manual toggle), removing BLE. The pairing token is suitable for a trusted LAN but does not replace TLS for an adversarial-LAN threat model.

## Further Considerations
1. **BLE+WiFi coexistence power/RAM cost untested on this hardware** — ESP32-S3 shares a single 2.4GHz radio between BT and WiFi via IDF's built-in coexistence scheduler; expect some throughput/latency impact when both are active but not a hard blocker. Verify it in Phase E rather than assuming.
2. **Difficulty estimate**: Firmware remains low-moderate effort because it largely restores a previously-built design; WebSocket client lifecycle, command outcome reporting, and token checks are the new firmware work. Android is the larger moderate-effort portion because it now needs network binding, runtime permissions, provisioning/token handling, discovery, and a transport-neutral panel boundary. This is a multi-session task, but it is now bounded by explicit contracts rather than exploratory R&D.
