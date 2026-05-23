#pragma once

#include "input/actions/actionsResponse.hpp"
#include "power/powerState.hpp"

namespace controlSystem
{
    enum class ActionCommandRoute
    {
        None,
        PowerStateTransition,
        IgnoreWhileNotOn,
        System,
        Relay,
        Display,
        Brightness,
        UartDispatch,
    };

    constexpr ActionCommandRoute classifyCommand(CommandId command,
                                                 ControlBoardPowerState powerState)
    {
        if (command == CMD_NO_ACTION)
        {
            return ActionCommandRoute::None;
        }

        if (command == CMD_SYS_POWER)
        {
            return ActionCommandRoute::PowerStateTransition;
        }

        if (powerState != ControlBoardPowerState::ON)
        {
            return ActionCommandRoute::IgnoreWhileNotOn;
        }

        if (command == CMD_SYS_RPI_SHUTDOWN || command == CMD_EXIT_ITEM)
        {
            return ActionCommandRoute::System;
        }

        if (command == CMD_TOGGLE_DAC_ON || command == CMD_TOGGLE_DAC_OFF)
        {
            return ActionCommandRoute::Relay;
        }

        if (command == CMD_DISPLAY_OFF || command == CMD_DISPLAY_ON)
        {
            return ActionCommandRoute::Display;
        }

        if (command == CMD_CYCLE_BRIGHTNESS)
        {
            return ActionCommandRoute::Brightness;
        }

        return ActionCommandRoute::UartDispatch;
    }
}