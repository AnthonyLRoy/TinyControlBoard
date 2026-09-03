#pragma once

#include <cstdint>
#include <functional>

// Forward declarations only — no NimBLE headers in this header.
namespace controlSystem { class ActionProcessor; struct SystemState; }
struct UartMessage;

namespace ble
{
    /// Lightweight BLE GATT peripheral server.
    ///
    /// Exposes a custom 128-bit service with characteristics:
    ///   CMD char       (WRITE | WRITE_NO_RESPONSE) — 2-byte LE CommandId
    ///   STATUS char    (READ  | NOTIFY)            — 3 bytes [powerState, bitmask_lo, bitmask_hi]
    ///   TRACK_PROGRESS (READ  | NOTIFY)            — 5 bytes [elapsed_lo, elapsed_hi, duration_lo, duration_hi, isPlaying]
    ///   LIBRARY        (READ  | NOTIFY)            — MSG_LIBRARY_ENTRY payload, pushed immediately (not the 200ms poll)
    ///   LIBRARY_CMD    (WRITE | WRITE_NO_RESPONSE) — 4 bytes [cmdId_lo, cmdId_hi, param_lo, param_hi]
    ///   PLAYLIST_CMD   (WRITE | WRITE_NO_RESPONSE) — [cmdId_lo, cmdId_hi, name(UTF-8, variable)]
    ///   PLAYLIST_RESULT(READ  | NOTIFY)            — MSG_PLAYLIST_RESULT payload, pushed immediately
    ///
    /// All NimBLE implementation details are confined to BleServer.cpp.
    /// Call start() once after ControlBoard::init() succeeds.
    class BleServer
    {
    public:
        void start(controlSystem::ActionProcessor &processor,
                   controlSystem::SystemState     &state,
                   std::function<void(uint16_t cmdId, uint16_t param)> onLibraryCommand,
                   std::function<void(uint16_t cmdId, const uint8_t *p_name, uint8_t nameLen)> onPlaylistCommand);
    };

    // Pushes a MSG_LIBRARY_ENTRY notification immediately (bypasses the 200ms poll task).
    void notifyLibraryEntry(const UartMessage &rMsg);

    // Pushes a MSG_PLAYLIST_RESULT notification immediately (bypasses the 200ms poll task).
    void notifyPlaylistResult(const UartMessage &rMsg);

} // namespace ble
