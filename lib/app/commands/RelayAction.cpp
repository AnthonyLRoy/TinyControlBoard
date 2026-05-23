#include "app/commands/RelayAction.hpp"
#include "app/ActionContext.hpp"
#include "protocol/uartProtocol.hpp"

namespace actions
{
    void RelayAction::execute(controlSystem::ActionContext &ctx)
    {
        ctx.relayController.handleToggleDac(command == CMD_TOGGLE_DAC_ON);
    }
}
