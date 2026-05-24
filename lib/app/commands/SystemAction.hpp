#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Handles system-level commands (RPi shutdown, exit-item).
    class SystemAction : public IAction
    {
    public:
        explicit SystemAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }
        void execute(controlSystem::ActionContext &ctx) override;
    };
}
