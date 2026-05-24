#pragma once

#include <atomic>
#include "power/powerState.hpp"

namespace controlSystem
{
    // Central, thread-safe runtime state. Uses only std::atomic — no FreeRTOS headers.
    struct SystemState
    {
        std::atomic<ControlBoardPowerState> powerState{ControlBoardPowerState::OFF};
    };
}
