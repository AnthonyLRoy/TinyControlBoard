#pragma once

#include "input/actions/IAction.hpp"
#include "app/ActionContext.hpp"
#include "indicators/ledManager.hpp"

namespace actions
{
    /// Drives a power-state transition (on, sleep, deep-sleep).
    /// Does not require the system to already be ON — it IS what turns it on.
    class PowerTransitionAction : public IAction
    {
    public:
        PowerTransitionAction(CommandId cmd, uint16_t releaseMs)
        {
            command = cmd;
            releaseTimeMillis = releaseMs;
        }

        bool requiresPowerOn() const override { return false; }

        void execute(controlSystem::ActionContext &ctx) override
        {
            ctx.powerHandler.handle(*this);
            ctx.systemState.powerState.store(indicators::getPowerLed().getState());
        }
    };
}
