#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Routes a command through the UART dispatcher for transmission to the connected host.
    class UartDispatchAction : public IAction
    {
    public:
        // Creates a UART-dispatch action for a single outbound command.
        explicit UartDispatchAction(CommandId cmd) { command = cmd; }

        // UART dispatch actions only run when the board is powered on.
        bool requiresPowerOn() const override { return true; }
        // Sends the mapped command to the UART dispatcher for outbound transmission.
        void execute(controlSystem::ActionContext &ctx) override;
    };
}
