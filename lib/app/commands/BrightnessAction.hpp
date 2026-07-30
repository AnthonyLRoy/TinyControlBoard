#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// manages brightness-related commands (e.g. toggle display, cycle brightness, etc.)
    class BrightnessAction : public IAction
    {
    public:
        explicit BrightnessAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }
        void execute(controlSystem::ActionContext &ctx) override;
    };


    
}
