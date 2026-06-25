#pragma once

#include "app/ActionCommandCatalog.hpp"
#include "app/ActionCommandRoute.hpp"
#include "power/powerState.hpp"

namespace controlSystem
{
    constexpr ActionCommandRoute classifyCommand(CommandId command,
                                                 ControlBoardPowerState powerState)
    {
        const auto route = classifyOnStateCommand(command);

        if (route == ActionCommandRoute::None ||
            route == ActionCommandRoute::PowerStateTransition)
        {
            return route;
        }

        if (powerState != ControlBoardPowerState::ON)
        {
            return ActionCommandRoute::IgnoreWhileNotOn;
        }

        return route;
    }
}