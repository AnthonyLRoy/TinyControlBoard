#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Routes DAC-related relay commands through the board relay controller.
    class RelayAction : public IAction
    {
    public:
        // Creates a relay action for a specific DAC or relay command.
        explicit RelayAction(CommandId cmd) { command = cmd; }

        // Relay commands require the board to be active before the action can run.
        bool requiresPowerOn() const override { return true; }
        // Executes the selected relay toggle in the shared action context.
        void execute(controlSystem::ActionContext &ctx) override;
    };
}
