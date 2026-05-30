#include "app/ActionUartDispatcher.hpp"
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

    namespace
    {
        // Toggle commands: a logical ON/OFF pair maps to one normalized wire command
        // with a boolean parameter. Adding a new toggle is one table row.
        struct ToggleMapping
        {
            CommandId onCmd;
            CommandId offCmd;
            CommandId wireCmd;
            const char *logTag;
        };

        constexpr ToggleMapping k_toggleTable[] = {
            {CMD_COVER_VIEW_ON,   CMD_COVER_VIEW_OFF,   CMD_TOGGLE_COVER_VIEW, "Cover_View"},
            {CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF, CMD_TOGGLE_METER,      "Meter"},
            {CMD_REPEAT_ON,       CMD_REPEAT_OFF,       CMD_TOGGLE_REPEAT,     "Repeat"},
            {CMD_RANDOM_ON,       CMD_RANDOM_OFF,       CMD_TOGGLE_RANDOM,     "Random"},
        };
    } // namespace

    bool ActionUartDispatcher::handle(const actions::IAction &action)
    {
        for (const auto &m : k_toggleTable)
        {
            if (action.command == m.onCmd || action.command == m.offCmd)
            {
                ESP_LOGI(k_logTag, "Processing %s toggle (%s)", m.logTag,
                         action.command == m.onCmd ? "ON" : "OFF");
                UartMessage message;
                message.commandId = m.wireCmd;
                message.params[0] = (action.command == m.onCmd) ? 1 : 0;
                mr_uartCommandSink.sendUartMessage(m.logTag, message);
                return true;
            }
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