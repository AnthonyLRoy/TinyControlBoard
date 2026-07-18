#pragma once

#include <atomic>
#include <cstdint>
#include "power/powerState.hpp"

namespace controlSystem
{
    // Central, thread-safe runtime state. Uses only std::atomic — no FreeRTOS headers.
    struct SystemState
    {
        std::atomic<ControlBoardPowerState> powerState{ControlBoardPowerState::OFF};
        // One bit per button (bit[n] = buttonId n). Updated by ControlBoard::setButtonLed.
        std::atomic<uint16_t> buttonLedBitmask{0};
    };
}
