#pragma once

#include "input/actions/IAction.hpp"
#include "app/ActionContext.hpp"
#include "indicators/ledManager.hpp"

namespace actions
{
    /// Advances the monitor brightness to the next step in the cycle.
    class BrightnessAction : public IAction
    {
    public:
        explicit BrightnessAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }

        void execute(controlSystem::ActionContext &) override
        {
            indicators::getMonitorBrightnessController().cycleBrightness();
        }
    };
}
