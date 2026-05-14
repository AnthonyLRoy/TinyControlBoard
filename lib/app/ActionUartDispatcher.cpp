#include "app/ActionUartDispatcher.hpp"

#if __has_include("esp_log.h")
#include "esp_log.h"
#else
#define ESP_LOGI(...)
#endif

namespace controlSystem
{
    const SimpleCommandEntry sSimpleCommands[] = {
        {"Next_Track", CMD_NEXT_TRACK},
        {"Prev_Track", CMD_PREVIOUS_TRACK},
        {"Play_Pause", CMD_PLAY_PAUSE},
        {"Stop", CMD_STOP_TRACK},
        {"Skip_Forward", CMD_SKIP_FORWARD},
        {"Skip_Back", CMD_SKIP_BACK},
        {"Item_Select", CMD_ITEM_SELECT},
    };

    const size_t kSimpleCommandCount = sizeof(sSimpleCommands) / sizeof(sSimpleCommands[0]);

    ActionUartDispatcher::ActionUartDispatcher(IUartCommandSink &rUartCommandSink)
        : mrUartCommandSink(rUartCommandSink)
    {
    }

    bool ActionUartDispatcher::handle(const actions::ActionResponse &response)
    {
        return handleCoverViewCommand(response) ||
               handleMeterCommand(response) ||
               handleRotaryCommand(response) ||
               handleRepeatCommand(response) ||
               handleSimpleCommand(response);
    }

    bool ActionUartDispatcher::handleCoverViewCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_COVER_VIEW_ON && response.command != CMD_COVER_VIEW_OFF)
        {
            return false;
        }

        ESP_LOGI(mspTag, "Processing Cover View Toggle Command (%s)",
                 response.command == CMD_COVER_VIEW_ON ? "ON" : "OFF");

        UartMessage message;
        message.commandId = CMD_TOGGLE_COVER_VIEW;
        message.params[0] = (response.command == CMD_COVER_VIEW_ON) ? 1 : 0;
        mrUartCommandSink.sendUartMessage("Cover_View", message);
        return true;
    }

    bool ActionUartDispatcher::handleMeterCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_TOGGLE_METER_ON && response.command != CMD_TOGGLE_METER_OFF)
        {
            return false;
        }

        ESP_LOGI(mspTag, "Processing Meter Toggle Command (%s)",
                 response.command == CMD_TOGGLE_METER_ON ? "ON" : "OFF");
        UartMessage message;
        message.commandId = CMD_TOGGLE_METER;
        message.params[0] = (response.command == CMD_TOGGLE_METER_ON) ? 1 : 0;
        mrUartCommandSink.sendUartMessage("Meter", message);
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
        mrUartCommandSink.sendUartMessage("Rotary", message);
        ESP_LOGI(mspTag, "Processing Rotary Action Command (%s)",
                 response.parameters[0] == 0 ? "LEFT" : "RIGHT");
        return true;
    }

    bool ActionUartDispatcher::handleRepeatCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_REPEAT_ON && response.command != CMD_REPEAT_OFF)
        {
            return false;
        }

        ESP_LOGI(mspTag, "Processing Repeat Toggle Command (%s)",
                 response.command == CMD_REPEAT_ON ? "ON" : "OFF");
        UartMessage message;
        message.commandId = CMD_TOGGLE_REPEAT;
        message.params[0] = (response.command == CMD_REPEAT_ON) ? 1 : 0;
        mrUartCommandSink.sendUartMessage("Repeat", message);
        return true;
    }
//simple commands are those that can be directly mapped to a single UART command without needing additional parameters or special handling
    bool ActionUartDispatcher::handleSimpleCommand(const actions::ActionResponse &response)
    {
        for (size_t cmdReference = 0; cmdReference < kSimpleCommandCount; cmdReference++)
        {
            if (sSimpleCommands[cmdReference].commandId == response.command)
            {
                mrUartCommandSink.sendUartCommand(sSimpleCommands[cmdReference].pLogTag, response.command);
                ESP_LOGI(mspTag, "Sending command: %s", sSimpleCommands[cmdReference].pLogTag);
                return true;
            }
        }

        return false;
    }
}