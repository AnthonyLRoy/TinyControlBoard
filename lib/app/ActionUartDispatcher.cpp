#include "app/ActionUartDispatcher.hpp"

#include "app/ActionCommandCatalog.hpp"
#include "protocol/commandCatalog.hpp"

#if __has_include("esp_log.h")
#include "esp_log.h"
#else
#define ESP_LOGI(...)
#endif

namespace controlSystem
{
    ActionUartDispatcher::ActionUartDispatcher(IUartCommandSink &rUartCommandSink)
        : mr_uartCommandSink(rUartCommandSink)
    {
    }

    bool ActionUartDispatcher::handle(const actions::IAction &action)
    {
        if (const auto *toggleSpec = findToggleCommandSpecByStateCommand(action.command);
            toggleSpec != nullptr &&
            toggleSpec->route == ActionCommandRoute::UartDispatch &&
            toggleSpec->p_uartLogTag != nullptr)
        {
            ESP_LOGI(k_logTag, "Processing %s toggle (%s)", toggleSpec->p_uartLogTag,
                     action.command == toggleSpec->onCommand ? "ON" : "OFF");
            UartMessage message;
            message.commandId = toggleSpec->semanticCommand;
            message.params[0] = (action.command == toggleSpec->onCommand) ? 1 : 0;
            mr_uartCommandSink.sendUartMessage(toggleSpec->p_uartLogTag, message);
            return true;
        }

        if (action.command == CMD_ROTARY_ACTION)
        {
            UartMessage message;
            message.commandId = action.command;
            message.params[0] = action.parameters[0];
            mr_uartCommandSink.sendUartMessage("Rotary", message);
            ESP_LOGI(k_logTag, "Processing Rotary Action Command (%s)",
                     action.parameters[0] == 0 ? "LEFT" : "RIGHT");
            return true;
        }

        const char *p_commandTag = getSimpleCommandLogTag(action.command);
        if (p_commandTag)
        {
            mr_uartCommandSink.sendUartCommand(p_commandTag, action.command);
            ESP_LOGI(k_logTag, "Sending command: %s", p_commandTag);
            return true;
        }

        return false;
    }
}