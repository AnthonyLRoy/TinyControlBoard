#include "app/commands/UartDispatchAction.hpp"
#include "app/ActionContext.hpp"

namespace actions
{
    // Routes the command through the UART dispatcher so it is sent to the connected Raspberry Pi or host.
    void UartDispatchAction::execute(controlSystem::ActionContext &ctx)
    {
        ctx.uartDispatcher.handle(*this);
    }
}
