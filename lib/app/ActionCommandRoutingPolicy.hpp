#pragma once

#include "app/ActionCommandRoute.hpp"
#include "protocol/uartProtocol.hpp"
#include "power/powerState.hpp"

namespace controlSystem
{
    namespace
    {
        struct CommandRouteEntry { CommandId command; ActionCommandRoute route; };

        // Add one row to classify any command that should NOT route to UartDispatch.
        // Commands absent from this table default to ActionCommandRoute::UartDispatch.
        constexpr CommandRouteEntry k_commandRouteTable[] = {
            { CMD_SYS_RPI_SHUTDOWN, ActionCommandRoute::System    },
            { CMD_EXIT_ITEM,        ActionCommandRoute::System    },
            { CMD_TOGGLE_DAC_ON,    ActionCommandRoute::Relay     },
            { CMD_TOGGLE_DAC_OFF,   ActionCommandRoute::Relay     },
            { CMD_TOGGLE_DAC,       ActionCommandRoute::Relay     },
            { CMD_CYCLE_BRIGHTNESS, ActionCommandRoute::Brightness},
            { CMD_TOGGLE_DISPLAY,   ActionCommandRoute::Brightness},
        };
    } // namespace

    constexpr ActionCommandRoute classifyCommand(CommandId command,
                                                 ControlBoardPowerState powerState)
    {
        if (command == CMD_NO_ACTION) return ActionCommandRoute::None;
        if (command == CMD_SYS_POWER) return ActionCommandRoute::PowerStateTransition;

        if (powerState != ControlBoardPowerState::ON)
            return ActionCommandRoute::IgnoreWhileNotOn;

        for (const auto &entry : k_commandRouteTable)
            if (entry.command == command)
                return entry.route;

        return ActionCommandRoute::UartDispatch;
    }
}