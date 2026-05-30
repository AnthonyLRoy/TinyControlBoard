#include "commandCatalog.hpp"

#include <cstddef>

namespace controlSystem
{

namespace
{
    struct CommandCatalogEntry
    {
        CommandId commandId;
        const char *p_commandName;
        const char *p_simpleLogTag;
    };

    const CommandCatalogEntry s_commandCatalog[] = {
        {CMD_NO_ACTION, "No_Action", nullptr},
        {CMD_SYS_POWER, "Sys_Power", nullptr},
        {CMD_SYS_RPI_SHUTDOWN, "Sys_Rpi_Shutdown", nullptr},
        {CMD_SYS_HEARTBEAT, "Sys_Heartbeat", nullptr},
        {CMD_SYS_NOHEARTBEAT, "Sys_NoHeartbeat", nullptr},
        {CMD_NEXT_TRACK, "Next_Track", "Next_Track"},
        {CMD_PREVIOUS_TRACK, "Prev_Track", "Prev_Track"},
        {CMD_PLAY_PAUSE, "Play_Pause", "Play_Pause"},
        {CMD_STOP_TRACK, "Stop", "Stop"},
        {CMD_SKIP_FORWARD, "Skip_Forward", "Skip_Forward"},
        {CMD_SKIP_BACK, "Skip_Back", "Skip_Back"},
        {CMD_PREV_MENU_ITEM, "Prev_Menu_Item", "Prev_Menu_Item"},
        {CMD_NEXT_MENU_ITEM, "Next_Menu_Item", "Next_Menu_Item"},
        {CMD_ITEM_SELECT, "Item_Select", "Item_Select"},
        {CMD_EXIT_ITEM, "Exit_Item", nullptr},
        {CMD_DISPLAY_OFF, "Display_Off", nullptr},
        {CMD_TOGGLE_METER_ON, "Toggle_Meter_On", nullptr},
        {CMD_TOGGLE_METER_OFF, "Toggle_Meter_Off", nullptr},
        {CMD_DISPLAY_ON, "Display_On", nullptr},
        {CMD_TOGGLE_DAC_ON, "Toggle_Dac_On", nullptr},
        {CMD_TOGGLE_DAC_OFF, "Toggle_Dac_Off", nullptr},
        {CMD_ROTARY_LEFT, "Rotary_Left", nullptr},
        {CMD_ROTARY_RIGHT, "Rotary_Right", nullptr},
        {CMD_ROTARY_ACTION, "Rotary_Action", nullptr},
        {CMD_TOGGLE_DAC, "Toggle_Dac", nullptr},
        {CMD_TOGGLE_DISPLAY, "Toggle_Display", nullptr},
        {CMD_TOGGLE_METER, "Toggle_Meter", nullptr},
        {CMD_CYCLE_BRIGHTNESS, "Cycle_Brightness", nullptr},
        {CMD_COVER_VIEW_ON, "Cover_View_On", nullptr},
        {CMD_COVER_VIEW_OFF, "Cover_View_Off", nullptr},
        {CMD_TOGGLE_COVER_VIEW, "Toggle_Cover_View", nullptr},
        {CMD_REPEAT_ON, "Repeat_On", nullptr},
        {CMD_REPEAT_OFF, "Repeat_Off", nullptr},
        {CMD_TOGGLE_REPEAT, "Toggle_Repeat", nullptr},
        {CMD_RANDOM_ON, "Random_On", nullptr},
        {CMD_RANDOM_OFF, "Random_Off", nullptr},
        {CMD_TOGGLE_RANDOM, "Toggle_Random", nullptr},
    };

    const size_t k_commandCatalogCount = sizeof(s_commandCatalog) / sizeof(s_commandCatalog[0]);
}

const char *getSimpleCommandLogTag(CommandId commandId)
{
    for (size_t i = 0; i < k_commandCatalogCount; ++i) {
        if (s_commandCatalog[i].commandId == commandId)
            return s_commandCatalog[i].p_simpleLogTag;
    }

    return nullptr;
}

const char *getCommandNameById(CommandId commandId)
{
    for (size_t i = 0; i < k_commandCatalogCount; ++i) {
        if (s_commandCatalog[i].commandId == commandId)
            return s_commandCatalog[i].p_commandName;
    }

    return "UNKNOWN";
}

} // namespace controlSystem
