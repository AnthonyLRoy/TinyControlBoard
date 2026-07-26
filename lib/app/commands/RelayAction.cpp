#include "app/commands/RelayAction.hpp"
#include "app/ActionContext.hpp"
#include "protocol/uartProtocol.hpp"

namespace actions
{
    void RelayAction::execute(controlSystem::ActionContext &ctx)
    {
        if (command == CMD_TOGGLE_DAC)
        {
            ctx.relayController.toggleDac();
            return;
        }

        ctx.relayController.handleToggleDac(command == CMD_TOGGLE_DAC_ON);
    }
}
