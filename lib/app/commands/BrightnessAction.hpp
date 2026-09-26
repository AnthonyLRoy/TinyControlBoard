#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Wraps the brightness-related command set, including display toggle and brightness stepping actions.
    class BrightnessAction : public IAction
    {
    public:
        // Creates a brightness action bound to a single command ID.
        explicit BrightnessAction(CommandId cmd) { command = cmd; }

        // Brightness actions are only valid while the board is powered on.
        bool requiresPowerOn() const override { return true; }
        // Executes the chosen brightness action using the board's brightness controller and shared context.
        void execute(controlSystem::ActionContext &ctx) override;
    };


    
}
