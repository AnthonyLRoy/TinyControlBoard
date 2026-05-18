#include "controlSystemHelpers.hpp"

#include "app/ActionUartDispatcher.hpp"

namespace controlSystem
{

namespace
{
    struct CommandNameEntry
    {
        CommandId commandId;
        const char *pCommandName;
    };

    const CommandNameEntry sAdditionalCommands[] = {
        {CMD_NO_ACTION, "No_Action"},
        {CMD_SYS_POWER, "Sys_Power"},
        {CMD_SYS_RPI_SHUTDOWN, "Sys_Rpi_Shutdown"},
        {CMD_SYS_HEARTBEAT, "Sys_Heartbeat"},
        {CMD_SYS_NOHEARTBEAT, "Sys_NoHeartbeat"},
        {CMD_PREV_MENU_ITEM, "Prev_Menu_Item"},
        {CMD_NEXT_MENU_ITEM, "Next_Menu_Item"},
        {CMD_EXIT_ITEM, "Exit_Item"},
        {CMD_DISPLAY_OFF, "Display_Off"},
        {CMD_DISPLAY_ON, "Display_On"},
        {CMD_TOGGLE_METER_ON, "Toggle_Meter_On"},
        {CMD_TOGGLE_METER_OFF, "Toggle_Meter_Off"},
        {CMD_TOGGLE_DAC_ON, "Toggle_Dac_On"},
        {CMD_TOGGLE_DAC_OFF, "Toggle_Dac_Off"},
        {CMD_ROTARY_LEFT, "Rotary_Left"},
        {CMD_ROTARY_RIGHT, "Rotary_Right"},
        {CMD_ROTARY_ACTION, "Rotary_Action"},
        {CMD_TOGGLE_DAC, "Toggle_Dac"},
        {CMD_TOGGLE_DISPLAY, "Toggle_Display"},
        {CMD_TOGGLE_METER, "Toggle_Meter"},
        {CMD_CYCLE_BRIGHTNESS, "Cycle_Brightness"},
        {CMD_COVER_VIEW_ON, "Cover_View_On"},
        {CMD_COVER_VIEW_OFF, "Cover_View_Off"},
        {CMD_TOGGLE_COVER_VIEW, "Toggle_Cover_View"},
        {CMD_REPEAT_ON, "Repeat_On"},
        {CMD_REPEAT_OFF, "Repeat_Off"},
        {CMD_TOGGLE_REPEAT, "Toggle_Repeat"},
        {CMD_RANDOM_ON, "Random_On"},
        {CMD_RANDOM_OFF, "Random_Off"},
        {CMD_TOGGLE_RANDOM, "Toggle_Random"},
    };

    const size_t kAdditionalCommandCount = sizeof(sAdditionalCommands) / sizeof(sAdditionalCommands[0]);
}

const char *getCommandNameById(CommandId commandId)
{
    for (size_t i = 0; i < kSimpleCommandCount; ++i) {
        if (sSimpleCommands[i].commandId == commandId)
            return sSimpleCommands[i].pLogTag;
    }

    for (size_t i = 0; i < kAdditionalCommandCount; ++i) {
        if (sAdditionalCommands[i].commandId == commandId)
            return sAdditionalCommands[i].pCommandName;
    }

    return "UNKNOWN";
}

} // namespace controlSystem
