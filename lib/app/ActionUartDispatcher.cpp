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
    namespace
    {
        constexpr const char *k_dispatchLogTag = "Uart_Dispatcher ";

        bool tryHandleToggleAction(transport::uart::IUartCommandSink &rUartCommandSink, const actions::IAction &action)
        {
            const auto *toggleSpec = findToggleCommandSpecByStateCommand(action.command);
            if (toggleSpec == nullptr ||
                toggleSpec->route != ActionCommandRoute::UartDispatch ||
                toggleSpec->p_uartLogTag == nullptr)
            {
                return false;
            }

            ESP_LOGI(k_dispatchLogTag, "Processing %s toggle (%s)", toggleSpec->p_uartLogTag,
                     action.command == toggleSpec->onCommand ? "ON" : "OFF");

            UartMessage message;
            message.commandId = toggleSpec->semanticCommand;
            message.params[0] = (action.command == toggleSpec->onCommand) ? 1 : 0;
            rUartCommandSink.sendUartMessage(toggleSpec->p_uartLogTag, message);
            return true;
        }

        bool tryHandleRotaryAction(transport::uart::IUartCommandSink &rUartCommandSink, const actions::IAction &action)
        {
            if (action.command != CMD_ROTARY_ACTION)
            {
                return false;
            }

            UartMessage message;
            message.commandId = action.command;
            message.params[0] = action.parameters[0];
            rUartCommandSink.sendUartMessage("Rotary", message);
            ESP_LOGI(k_dispatchLogTag, "Processing Rotary Action Command (%s)",
                     action.parameters[0] == 0 ? "LEFT" : "RIGHT");
            return true;
        }

        bool tryHandleSimpleCommand(transport::uart::IUartCommandSink &rUartCommandSink, const actions::IAction &action)
        {
            const char *p_commandTag = getSimpleCommandLogTag(action.command);
            if (p_commandTag == nullptr)
            {
                return false;
            }

            rUartCommandSink.sendUartCommand(p_commandTag, action.command);
            ESP_LOGI(k_dispatchLogTag, "Sending command: %s", p_commandTag);
            return true;
        }
    } // namespace

    ActionUartDispatcher::ActionUartDispatcher(transport::uart::IUartCommandSink &rUartCommandSink)
        : mr_uartCommandSink(rUartCommandSink)
    {
    }

    bool ActionUartDispatcher::handle(const actions::IAction &action)
    {
        return tryHandleToggleAction(mr_uartCommandSink, action) ||
               tryHandleRotaryAction(mr_uartCommandSink, action) ||
               tryHandleSimpleCommand(mr_uartCommandSink, action);
    }
}