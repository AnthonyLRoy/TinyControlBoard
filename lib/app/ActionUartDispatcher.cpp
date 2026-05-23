#include "app/ActionUartDispatcher.hpp"
#include "support/controlSystemHelpers.hpp"

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

    bool ActionUartDispatcher::handle(const actions::ActionResponse &response)
    {
        return handleCoverViewCommand(response) ||
               handleMeterCommand(response) ||
               handleRotaryCommand(response) ||
               handleRepeatCommand(response) ||
               handleRandomCommand(response) ||
               handleSimpleCommand(response);
    }

    bool ActionUartDispatcher::handleCoverViewCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_COVER_VIEW_ON && response.command != CMD_COVER_VIEW_OFF)
        {
            return false;
        }

        ESP_LOGI(k_logTag, "Processing Cover View Toggle Command (%s)",
                 response.command == CMD_COVER_VIEW_ON ? "ON" : "OFF");

        UartMessage message;
        message.commandId = CMD_TOGGLE_COVER_VIEW;
        message.params[0] = (response.command == CMD_COVER_VIEW_ON) ? 1 : 0;
        mr_uartCommandSink.sendUartMessage("Cover_View", message);
        return true;
    }

    bool ActionUartDispatcher::handleMeterCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_TOGGLE_METER_ON && response.command != CMD_TOGGLE_METER_OFF)
        {
            return false;
        }

        ESP_LOGI(k_logTag, "Processing Meter Toggle Command (%s)",
                 response.command == CMD_TOGGLE_METER_ON ? "ON" : "OFF");
        UartMessage message;
        message.commandId = CMD_TOGGLE_METER;
        message.params[0] = (response.command == CMD_TOGGLE_METER_ON) ? 1 : 0;
        mr_uartCommandSink.sendUartMessage("Meter", message);
        return true;
    }

    bool ActionUartDispatcher::handleRotaryCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_ROTARY_ACTION)
        {
            return false;
        }

        UartMessage message;
        message.commandId = response.command;
        message.params[0] = response.parameters[0];
        mr_uartCommandSink.sendUartMessage("Rotary", message);
        ESP_LOGI(k_logTag, "Processing Rotary Action Command (%s)",
                 response.parameters[0] == 0 ? "LEFT" : "RIGHT");
        return true;
    }

    bool ActionUartDispatcher::handleRepeatCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_REPEAT_ON && response.command != CMD_REPEAT_OFF)
        {
            return false;
        }

        ESP_LOGI(k_logTag, "Processing Repeat Toggle Command (%s)",
                 response.command == CMD_REPEAT_ON ? "ON" : "OFF");
        UartMessage message;
        message.commandId = CMD_TOGGLE_REPEAT;
        message.params[0] = (response.command == CMD_REPEAT_ON) ? 1 : 0;
        mr_uartCommandSink.sendUartMessage("Repeat", message);
        return true;
    }
//simple commands are those that can be directly mapped to a single UART command without needing additional parameters or special handling
    bool ActionUartDispatcher::handleRandomCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_RANDOM_ON && response.command != CMD_RANDOM_OFF)
        {
            return false;
        }

        ESP_LOGI(k_logTag, "Processing Random Toggle Command (%s)",
                 response.command == CMD_RANDOM_ON ? "ON" : "OFF");
        UartMessage message;
        message.commandId = CMD_TOGGLE_RANDOM;
        message.params[0] = (response.command == CMD_RANDOM_ON) ? 1 : 0;
        mr_uartCommandSink.sendUartMessage("Random", message);
        return true;
    }

    bool ActionUartDispatcher::handleSimpleCommand(const actions::ActionResponse &response)
    {
        const char *p_commandTag = getSimpleCommandLogTag(response.command);
        if (p_commandTag)
        {
            mr_uartCommandSink.sendUartCommand(p_commandTag, response.command);
            ESP_LOGI(k_logTag, "Sending command: %s", p_commandTag);
            return true;
        }

        return false;
    }
}