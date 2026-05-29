#pragma once

#include "app/ActionCommandRoute.hpp"
#include "protocol/uartProtocol.hpp"
#include "power/powerState.hpp"

namespace controlSystem
{
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

        if (command == CMD_CYCLE_BRIGHTNESS || command == CMD_TOGGLE_DISPLAY)
        {
            return ActionCommandRoute::Brightness;
        }

        return ActionCommandRoute::UartDispatch;
    }
}