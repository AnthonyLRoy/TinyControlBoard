#pragma once

#include "input/actions/IAction.hpp"
#include "app/ActionContext.hpp"
#include "protocol/uartProtocol.hpp"

#if __has_include("esp_log.h")
#include "esp_log.h"
#else
#define ESP_LOGI(...)
#endif

namespace actions
{
    /// Handles system-level commands (RPi shutdown, exit-item).
    class SystemAction : public IAction
    {
    public:
        explicit SystemAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }

        void execute(controlSystem::ActionContext &ctx) override
        {
            if (command == CMD_SYS_RPI_SHUTDOWN)
            {
                ctx.serial.sendUartCommand("RPi_Shutdown", CMD_SYS_RPI_SHUTDOWN);
                ctx.relayController.shutdownRpi(true);
            }
            // CMD_EXIT_ITEM: intentionally a no-op beyond logging
            ESP_LOGI("System_Action", "System command executed: 0x%04X", command);
        }
    };
}
