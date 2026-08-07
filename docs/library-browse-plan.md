# Plan: Library browse + play-track screen (Android)

Add a folder-drill-down music library browser to the Android app. Tapping the
now-playing text opens it at the library root; tapping a folder descends into it;
tapping a track adds it to the end of the current playlist and returns to the main
screen (does not clear the queue or force playback to jump to it). Implemented as a
new UART message type + two new command IDs, a new BLE characteristic pair, RPi-side
MPD browsing, and a new Android activity. Design keeps BLE/UART payloads tiny by
having the RPi hold the current directory listing and only exchanging integer
indices with ESP32/Android (never raw paths).

## Steps

### Phase 1 — Protocol layer
1. `lib/protocol/uartProtocol.hpp`: add `MSG_LIBRARY_ENTRY = 0x07`; add
   `CMD_BROWSE_REQUEST = 0x0128` (param0 = child index, or sentinel
   `k_browseUp=0xFFFE` / `k_browseRoot=0xFFFF`) and `CMD_ADD_TRACK = 0x0129`
   (param0 = index of file in current listing); add `k_maxLibraryNameLen = 55`;
   add `UartMessage` fields `libraryEntryType/Index/Total/Name/NameLen`.
2. `lib/protocol/uartProtocol.cpp`: add a `MSG_LIBRARY_ENTRY` branch to
   `deserializeMessage()` (parses `[type(1), index(u16), total(u16), name]`),
   mirroring the existing `MSG_NOW_PLAYING` branch. No changes needed to
   `serializeMessage()` — outgoing `CMD_BROWSE_REQUEST`/`CMD_ADD_TRACK` reuse the
   existing fixed `params[5]` wire format. *(depends on step 1)*

### Phase 2 — Firmware BLE + wiring (*depends on Phase 1*)
3. `lib/ble/BleServer.hpp`/`.cpp`: add two characteristics — `LIBRARY_CHAR`
   (READ|NOTIFY, UUID `...9d05`) and `LIBRARY_CMD_CHAR` (WRITE|WRITE_NO_RSP, UUID
   `...9d06`, 4-byte payload `[cmdId_u16, param_u16]`). `BleServer::start()` gains a
   `std::function<void(uint16_t cmdId, uint16_t param)>` parameter invoked by the new
   write handler (keeps `lib/ble` decoupled from the UART transport type). Add a free
   function `ble::notifyLibraryEntry(const UartMessage&)` that pushes a notification
   **immediately** (not via the existing 200ms poll task — a whole directory listing
   at that cadence would be too slow for good UX).
4. `lib/app/ControlBoard.hpp`/`.cpp`: add `setLibraryEntryCallback(std::function<void(const UartMessage&)>)`;
   `handleSerialRxMessage` gets a `MSG_LIBRARY_ENTRY` branch that invokes it. Keeps
   `lib/app` from depending on `lib/ble` directly (one-way wiring done in `main.cpp`).
5. `src/main.cpp`: pass a lambda into `bleServer.start(...)` that builds a `UartMessage`
   (`msgType=MSG_COMMAND`, `commandId`, `params[0]=param`) and sends it via
   `transport::uart::UartTransport::getInstance().sendUartMessage(...)`; wire
   `board.setLibraryEntryCallback([](const UartMessage& m){ ble::notifyLibraryEntry(m); });`
   after `bleServer.start()`.
6. Deliberately bypass `ActionProcessor`/`ActionUartDispatcher` for these two new
   commands — that pipeline is built for physical-button semantics (SPI LED
   bookkeeping, toggle state); browse/select are remote-only and go straight from
   the BLE write handler to a UART send.

### Phase 3 — Raspberry Pi listener (*can run parallel with Phase 2*)
7. `scripts/rpi/home/antho/uart5_listener.py`: add a minimal hand-rolled MPD client
   over its plain TCP protocol (`localhost:6600`, `lsinfo "<path>"`) — more reliable
   than parsing `mpc`'s CLI output, which doesn't clearly distinguish folders from
   files in plain-text mode. Track `_browse_path`/`_browse_entries` module state.
   `CMD_BROWSE_REQUEST` handler updates the path (root/up/descend) and re-lists,
   streaming one `MSG_LIBRARY_ENTRY` packet per row (cap 200 entries per listing,
   plus an empty-listing sentinel `entryType=2` for zero-entry folders).
   `CMD_ADD_TRACK` handler runs `add "<path>"` (appends to the end of the current
   MPD queue, does **not** clear it or change playback) for the file at that index.
   Add outbound packet building (`build_packet`, matching `heartbeat_sender.py`'s
   framing) since this script currently only reads.
8. **Correctness fix**: currently only `heartbeat_sender.py` ever writes to the shared
   `/dev/ttyAMA5` port + drives the shared GPIO24 "data ready" line. Adding a second
   writer (`uart5_listener.py`, for listing replies) creates a real write race. Add a
   `fcntl.flock`-based lock file (`/tmp/tinycontrolboard_uart.lock`) around the
   send-critical section in **both** `uart5_listener.py` and
   `scripts/rpi/home/antho/heartbeat_sender.py`.

### Phase 4 — Android app (*depends on Phase 1 for wire format; can start once UUIDs are fixed*)
9. `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ble/BleUuids.kt`: add
   `LIBRARY_CHAR` (`...9d05`) and `LIBRARY_CMD_CHAR` (`...9d06`).
10. New `model/LibraryEntry.kt`: `data class LibraryEntry(val index: Int, val total: Int, val isDirectory: Boolean, val name: String)`.
11. `BoardBleManager.kt`: subscribe to `LIBRARY_CHAR` (enqueue pattern like the other
    three characteristics); parse notifications into `LibraryEntry`, accumulate into
    `_libraryListing: MutableStateFlow<List<LibraryEntry>>` (reset when `index==0`
    starts a new listing); add `fun browseInto(index: Int)`, `browseUp()`,
    `browseRoot()`, `addTrack(index: Int)` that write the 4-byte
    `[cmdId, param]` payload to `LIBRARY_CMD_CHAR` (write-no-response).
12. `MainViewModel.kt`: expose `libraryListing` (pass-through `StateFlow`) and the four
    browse/select functions.
13. `MainActivity.kt`: `b.tvNowPlaying.setOnClickListener { startActivity(Intent(this, LibraryActivity::class.java)) }`.
14. New `ui/LibraryActivity.kt` + `ui/LibraryAdapter.kt` + layouts
    `res/layout/activity_library.xml` (toolbar + `RecyclerView`, modeled on
    `activity_view_selection.xml`) and `res/layout/item_library_entry.xml` (modeled on
    `item_ble_device.xml`). Adapter renders an "Up" row when not at root (local-only,
    calls `browseUp()`, not sent as a real index), folder rows (tap → `browseInto`),
    and track rows (tap → `addTrack`, then shows a brief "Added to playlist" toast/
    Snackbar and stays on the listing so the user can keep adding more tracks —
    it does **not** `finish()` back to `MainActivity`).
    Calls `vm.browseRoot()` in `onCreate`.
15. New vector drawables `ic_folder.xml` / `ic_music_note.xml` / `ic_arrow_up.xml`
    (pattern-match `ic_chevron_right.xml`); register `LibraryActivity` in
    `AndroidManifest.xml`; add any new strings to `strings.xml`.

## Relevant files
- `lib/protocol/uartProtocol.hpp` / `.cpp` — new message type, command IDs, struct fields, deserialize branch
- `lib/ble/BleServer.hpp` / `.cpp` — two new GATT characteristics, immediate-push notify, `start()` signature change
- `lib/app/ControlBoard.hpp` / `.cpp` — `MSG_LIBRARY_ENTRY` dispatch to a callback
- `src/main.cpp` — wires BLE write callback → UART send, and UART receive callback → BLE notify
- `scripts/rpi/home/antho/uart5_listener.py` — MPD `lsinfo` client, browse/select handlers, outbound send capability
- `scripts/rpi/home/antho/heartbeat_sender.py` — add matching `fcntl` lock around its existing send path
- `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ble/BleUuids.kt`, `BoardBleManager.kt`
- `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/model/LibraryEntry.kt` (new)
- `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/viewmodel/MainViewModel.kt`
- `android/TinyRemote/app/src/main/kotlin/com/tinycb/remote/ui/MainActivity.kt`, `LibraryActivity.kt` (new), `LibraryAdapter.kt` (new)
- `android/TinyRemote/app/src/main/res/layout/activity_library.xml`, `item_library_entry.xml` (new)
- `android/TinyRemote/app/src/main/res/drawable/ic_folder.xml`, `ic_music_note.xml`, `ic_arrow_up.xml` (new)
- `android/TinyRemote/app/src/main/AndroidManifest.xml`, `res/values/strings.xml`

## Verification
1. PlatformIO Build task (firmware compiles, new BLE characteristics registered without RAM/flash regressions).
2. `python -m py_compile` on both modified RPi scripts (no syntax errors; no serial hardware available to fully exercise here).
3. Android `assembleDebug` via Gradle.
4. Manual/hardware verification (post-flash): open Library from the now-playing tap, confirm folder drill-down and "Up" navigation work, confirm tapping a track appends it to the end of the current queue without interrupting current playback, confirm now-playing/progress still update normally afterward.

## Decisions
- Whole-library folder browsing (not just current queue) — uses MPD's native directory tree via `lsinfo`, not `mpc playlist`.
- Tapping a track appends it to the end of the current queue (`add`) — it does **not** clear the queue or change what's currently playing.
- Entry point is tapping the now-playing text.
- Cap of 200 entries applies **per directory listing** (not the whole library) — no practical limit for a real library.
- Browse/select bypass the existing `ActionProcessor`/`ActionUartDispatcher` physical-button pipeline entirely.
- Library entries are pushed to BLE immediately on UART receipt, not through the existing 200ms status-poll task.

## Further Considerations
1. The dual-writer race on the shared serial port/GPIO24 line (Phase 3, step 8) is a **pre-existing latent issue** this feature would newly expose (today only `heartbeat_sender.py` ever sends). Recommend fixing it as part of this change via a shared `fcntl.flock` lock file.
2. `scripts/rpi/home/antho/UAart5Listener.py` (capital-letter variant) appears to be an older/duplicate copy not referenced by docs — this plan only touches the lowercase `uart5_listener.py`; flag separately if it should be deleted.
