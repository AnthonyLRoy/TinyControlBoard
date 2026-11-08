#include "app/commands/SystemAction.hpp"
#include "app/ActionContext.hpp"
#include "esp_log.h"

namespace actions
{
    // Executes system-level commands such as Pi shutdown and other controller actions through the shared action context.
    void SystemAction::execute(controlSystem::ActionContext &ctx)
    {
        if (command == CMD_SYS_RPI_SHUTDOWN)
        {
            ctx.brightnessController.clearDisplayOffMode();
            ctx.systemState.powerState.store(ctx.powerHandler.handle(*this));
        }
        // CMD_EXIT_ITEM: intentionally a no-op beyond logging
        ESP_LOGI("System_Action", "System command executed: 0x%04X", command);
    }
}
