#pragma once

#include "input/actions/IAction.hpp"
#include "app/ActionContext.hpp"
#include "protocol/uartProtocol.hpp"

namespace actions
{
    /// Toggles the DAC relay on or off.
    class RelayAction : public IAction
    {
    public:
        explicit RelayAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }

        void execute(controlSystem::ActionContext &ctx) override
        {
            ctx.relayController.handleToggleDac(command == CMD_TOGGLE_DAC_ON);
        }
    };
}
