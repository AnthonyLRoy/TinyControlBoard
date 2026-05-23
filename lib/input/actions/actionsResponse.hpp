#pragma once

#include "protocol/uartProtocol.hpp"
#include "app/ActionCommandRoute.hpp"

namespace actions
{
    /// A fully-classified action intent created from a button event.
    /// The `route` field is set by ControlBoardInputDispatcher after the
    /// producer fires, so ActionProcessor can dispatch without re-classifying.
    struct Action
    {
        controlSystem::ActionCommandRoute route = controlSystem::ActionCommandRoute::None;
        bool isActive = false;
        CommandId command = CMD_NO_ACTION;
        uint16_t parameters[5]{0, 0, 0, 0, 0};
        uint16_t releaseTimeMillis = 0;

        Action() = default;
    };
}