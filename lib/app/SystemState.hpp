#pragma once

#include <atomic>
#include "power/powerState.hpp"
#include "app/ControlBoardButtonIds.hpp"

namespace controlSystem
{
    // Central, thread-safe runtime state. Uses only std::atomic — no FreeRTOS headers.
    // buttonLedBitmask stores one bit per button LED (bit N = button index N).
    // Using an atomic allows safe read from any task without a mutex.
    struct SystemState
    {
        std::atomic<ControlBoardPowerState> powerState{ControlBoardPowerState::OFF};
        std::atomic<uint16_t> buttonLedBitmask{0};
    };
}
