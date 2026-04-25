#pragma once

#include "power/powerState.hpp"
#include <cstdint>

namespace controlSystem
{
    enum class PowerTransitionAction
    {
        None,
        PowerOn,
        Sleep,
        DeepSleep,
    };

    class PowerStateTransitionPolicy
    {
    public:
        static constexpr uint32_t kLongPressThresholdMs = 3000;

        static PowerTransitionAction evaluate(ControlBoardPowerState powerState,
                                              uint16_t releaseTimeMillis)
        {
            if (powerState == ControlBoardPowerState::OFF ||
                powerState == ControlBoardPowerState::SLEEP ||
                powerState == ControlBoardPowerState::DEEPSLEEP)
            {
                return PowerTransitionAction::PowerOn;
            }

            if (powerState != ControlBoardPowerState::ON)
            {
                return PowerTransitionAction::None;
            }

            return releaseTimeMillis < kLongPressThresholdMs
                ? PowerTransitionAction::Sleep
                : PowerTransitionAction::DeepSleep;
        }
    };
}