#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Handles system-level commands such as Raspberry Pi shutdown and other board-wide actions.
    class SystemAction : public IAction
    {
    public:
        // Creates a system command action bound to its command ID.
        explicit SystemAction(CommandId cmd) { command = cmd; }

        // System actions are only valid while the board is operating.
        bool requiresPowerOn() const override { return true; }
        // Executes the configured system action using the board's runtime services.
        void execute(controlSystem::ActionContext &ctx) override;
    };
}
