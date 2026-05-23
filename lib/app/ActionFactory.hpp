#pragma once

#include "input/actions/IAction.hpp"
#include <memory>

namespace controlSystem
{
    /// Creates the appropriate concrete IAction for the given command.
    /// Returns nullptr for CMD_NO_ACTION.
    /// Called by ActionProcessor::process() to obtain an executable action
    /// from the data-only Action DTO produced by the button pipeline.
    std::unique_ptr<actions::IAction> createAction(CommandId command,
                                                   uint16_t releaseTimeMs = 0);
}
