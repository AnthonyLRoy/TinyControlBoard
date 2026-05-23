#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Toggles the DAC relay on or off.
    class RelayAction : public IAction
    {
    public:
        explicit RelayAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }
        void execute(controlSystem::ActionContext &ctx) override;
    };
}
