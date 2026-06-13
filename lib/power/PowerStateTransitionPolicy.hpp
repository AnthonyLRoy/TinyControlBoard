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

    inline constexpr uint32_t kLongPressThresholdMs = 3000;

    constexpr ControlBoardPowerState getTransitionEntryState(PowerTransitionAction transition,
                                                             ControlBoardPowerState currentState)
    {
        switch (transition)
        {
        case PowerTransitionAction::PowerOn:
            return ControlBoardPowerState::TURNING_ON;
        case PowerTransitionAction::Sleep:
        case PowerTransitionAction::DeepSleep:
            return ControlBoardPowerState::SHUTTING_DOWN;
        case PowerTransitionAction::None:
        default:
            return currentState;
        }
    }

    constexpr ControlBoardPowerState getPostShutdownTransitionState(PowerTransitionAction transition,
                                                                    ControlBoardPowerState currentState)
    {
        switch (transition)
        {
        case PowerTransitionAction::Sleep:
            return ControlBoardPowerState::GOING_TO_SLEEP;
        case PowerTransitionAction::DeepSleep:
            return ControlBoardPowerState::GOING_INTO_DEEP_SLEEP;
        case PowerTransitionAction::None:
        case PowerTransitionAction::PowerOn:
        default:
            return currentState;
        }
    }

    constexpr PowerTransitionAction evaluatePowerTransition(ControlBoardPowerState powerState,
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
}