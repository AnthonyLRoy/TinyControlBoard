#include "app/commands/UartDispatchAction.hpp"
#include "app/ActionContext.hpp"

namespace actions
{
    void UartDispatchAction::execute(controlSystem::ActionContext &ctx)
    {
        ctx.uartDispatcher.handle(*this);
    }
}
