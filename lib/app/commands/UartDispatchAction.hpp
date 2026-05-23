#pragma once

#include "input/actions/IAction.hpp"
#include "app/ActionContext.hpp"

namespace actions
{
    /// Routes the command to the UART dispatcher for transmission to the host.
    class UartDispatchAction : public IAction
    {
    public:
        explicit UartDispatchAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }

        void execute(controlSystem::ActionContext &ctx) override
        {
            ctx.uartDispatcher.handle(*this);
        }
    };
}
