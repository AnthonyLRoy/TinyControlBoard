#pragma once

// Forward declarations only — no NimBLE headers in this header.
namespace controlSystem { class ActionProcessor; struct SystemState; }

namespace ble
{
    /// Lightweight BLE GATT peripheral server.
    ///
    /// Exposes a custom 128-bit service with two characteristics:
    ///   CMD char    (WRITE | WRITE_NO_RESPONSE) — 2-byte LE CommandId
    ///   STATUS char (READ  | NOTIFY)            — 3 bytes [powerState, bitmask_lo, bitmask_hi]
    ///
    /// All NimBLE implementation details are confined to BleServer.cpp.
    /// Call start() once after ControlBoard::init() succeeds.
    class BleServer
    {
    public:
        void start(controlSystem::ActionProcessor &processor,
                   controlSystem::SystemState     &state);
    };

} // namespace ble
