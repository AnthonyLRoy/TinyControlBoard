#pragma once

#include "input/actions/IAction.hpp"
#include "app/ActionContext.hpp"
#include "indicators/ledManager.hpp"

namespace actions
{
    /// Blanks or unblanks the connected display.
    class DisplayAction : public IAction
    {
    public:
        explicit DisplayAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }

        void execute(controlSystem::ActionContext &) override
        {
            indicators::getMonitorBrightnessController().setBlanked(command == CMD_DISPLAY_OFF);
        }
    };
}
