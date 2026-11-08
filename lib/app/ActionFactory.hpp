#pragma once

#include "input/actions/IAction.hpp"
#include "app/ActionCommandRoute.hpp"
#include <memory>

namespace controlSystem
{
    /// Template specialization for creating actions based on route type.
    /// Specialize this template for each ActionCommandRoute variant.
    template<ActionCommandRoute Route>
    std::unique_ptr<actions::IAction> createActionForRoute(CommandId command, uint16_t releaseTimeMs);

    /// Creates the appropriate concrete IAction for the given command.
    /// Returns nullptr for CMD_NO_ACTION.
    /// Called by ActionProcessor::process() to obtain an executable action
    /// from the data-only Action DTO produced by the button pipeline.
    std::unique_ptr<actions::IAction> createAction(CommandId command,
                                                   uint16_t releaseTimeMs = 0);
}
