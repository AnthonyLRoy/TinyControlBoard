#include "app/ActionUartDispatcher.hpp"

#if __has_include("esp_log.h")
#include "esp_log.h"
#else
#define ESP_LOGI(...)
#endif

namespace controlSystem
{
    const CommandConfig commandConfigs[] = {
        {"POWERCOMMAND", CMD_SYS_POWER},
        {"NEXTTRACK", CMD_NEXT_TRACK},
        {"PREVTRACK", CMD_PREVIOUS_TRACK},
        {"PLAYPAUSE", CMD_PLAY_PAUSE},
        {"STOP", CMD_STOP_TRACK},
        {"SKIPFORWARD", CMD_SKIP_FORWARD},
        {"SKIPBACK", CMD_SKIP_BACK},
        {"COVER", CMD_TOGGLE_COVER_VIEW},
        {"NEXTMENU", CMD_NEXT_MENU_ITEM},
        {"ITEMSELECT", CMD_ITEM_SELECT},
        {"DISPLAYOFF", CMD_DISPLAY_OFF},
        {"METERON", CMD_TOGGLE_METER_ON},
        {"METEROFF", CMD_TOGGLE_METER_OFF},
        {"DISPLAYON", CMD_DISPLAY_ON},
        {"ROTARY", CMD_ROTARY_ACTION},
        {"TOGGLEDAC", CMD_TOGGLE_DAC},
        {"TOGGLEDISPLAY", CMD_TOGGLE_DISPLAY},
        {"TOGGLEMETER", CMD_TOGGLE_METER},
        {"CYCLEBRIGHTNESS", CMD_CYCLE_BRIGHTNESS}
    };

    const size_t NUM_COMMANDS = sizeof(commandConfigs) / sizeof(commandConfigs[0]);

    ActionUartDispatcher::ActionUartDispatcher(IUartCommandSink &rUartCommandSink)
        : mrUartCommandSink(rUartCommandSink)
    {
    }

    bool ActionUartDispatcher::handle(const actions::ActionResponse &response)
    {
        return handleCoverViewCommand(response) ||
               handleMeterCommand(response) ||
               handleRotaryCommand(response) ||
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
        mrUartCommandSink.sendUartMessage("COVERVIEW", message);
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
        mrUartCommandSink.sendUartMessage("METER", message);
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
        mrUartCommandSink.sendUartMessage("ROTARY", message);
        ESP_LOGI(mspTag, "Processing Rotary Action Command (%s)",
                 response.parameters[0] == 0 ? "LEFT" : "RIGHT");
        return true;
    }

    bool ActionUartDispatcher::handleSimpleCommand(const actions::ActionResponse &response)
    {
        for (size_t cmdReference = 0; cmdReference < NUM_COMMANDS; cmdReference++)
        {
            if (commandConfigs[cmdReference].commandId == response.command)
            {
                mrUartCommandSink.sendUartCommand(commandConfigs[cmdReference].pLogTag, response.command);
                ESP_LOGI(mspTag, "Sending command: %s", commandConfigs[cmdReference].pLogTag);
                return true;
            }
        }

        return false;
    }
}