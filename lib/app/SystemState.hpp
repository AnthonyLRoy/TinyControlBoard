#pragma once

#include <atomic>
#include "power/powerState.hpp"
#include "app/ControlBoardButtonIds.hpp"

namespace controlSystem
{
    // Central, thread-safe runtime state. Uses only std::atomic — no FreeRTOS headers.
    // buttonLedStates is written exclusively from the action task (Phase 1 queue),
    // so no locking is required for that array.
    struct SystemState
    {
        std::atomic<ControlBoardPowerState> powerState{ControlBoardPowerState::OFF};
        bool buttonLedStates[controlBoardButtons::k_count]{};
    };
}
