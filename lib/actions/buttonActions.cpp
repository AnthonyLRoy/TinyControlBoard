
#include "buttonActions.hpp"
#include "SimpleCommandAction.hpp"
#include <esp_timer.h>
namespace actions
{

    static SimpleCommandAction PreviousTrackCmd(CMD_PREVIOUS_TRACK);
    static SimpleCommandAction NextTrackCmd(CMD_NEXT_TRACK);
    static SimpleCommandAction SkipForwardCmd(CMD_SKIP_FORWARD);
    static SimpleCommandAction SkipBackCmd(CMD_SKIP_BACK);
    static SimpleCommandAction PauseCmd(CMD_PLAY_PAUSE);
    static SimpleCommandAction StopCmd(CMD_STOP_TRACK);
    static SimpleCommandAction PreviousMenuCmd(CMD_PREV_MENU_ITEM);
    static SimpleCommandAction NextMenuCmd(CMD_NEXT_MENU_ITEM);
    static SimpleCommandAction MenuSelectCmd(CMD_ITEM_SELECT);

    // References to the shared simple actions
    ButtonAction& PreviousTrackInstance = PreviousTrackCmd;
    ButtonAction& NextTrackInstance = NextTrackCmd;
    ButtonAction& SkipForwardInstance = SkipForwardCmd;
    ButtonAction& SkipBackInstance = SkipBackCmd;
    ButtonAction& PauseInstance = PauseCmd;
    ButtonAction& StopInstance = StopCmd;
    ButtonAction& PreviousMenuInstance = PreviousMenuCmd;
    ButtonAction& NextMenuInstance = NextMenuCmd;
    ButtonAction& MenuSelectInstance = MenuSelectCmd;


    PowerButton PowerButtonInstance;
    ToggleDac ToggleDacInstance;
    SwitchOffDisplay SwitchOffDisplayInstance;
    ToggleMeterDisplay ToggleMeterDisplayInstance;
    RotaryMove RotaryMoveInstance;

    actionResponse ToggleDac::execute(bool pressed)
    {
        static bool state = 0;
        actionResponse response;
        if (pressed == true)
        { if( state == false)
            response.command = CMD_TOGGLE_DAC_ON;
            else
            response.command = CMD_TOGGLE_DAC_OFF;
            state = !state;
        }
        return response;
    }

    actionResponse SwitchOffDisplay::execute(bool pressed)
    {
        static bool state = 0;
        actionResponse response;
        if (pressed == true)
        { if( state == false)
            response.command = CMD_DISPLAY_OFF;
            else
            response.command = CMD_DISPLAY_ON;
            state = !state;
        }
        return response;
    }

    actionResponse ToggleMeterDisplay::execute(bool pressed)
    {
        static bool state = 0;
        actionResponse response;
        if (pressed == true)
        { if( state == false)
            response.command = CMD_TOGGLE_METER_ON;
            else
            response.command = CMD_TOGGLE_METER_OFF;
            state = !state;
        }
        return response;
    }
    
    actionResponse RotaryMove::execute(bool pressed)
    {
        actionResponse response;
        if (pressed)
        {
            response.command = CMD_ROTARY_RIGHT; // Example command for rotary movement
        }
        else
        {
            response.command = CMD_ROTARY_LEFT; // Example command for rotary movement
        }
        return response;
    }

    actionResponse PowerButton::execute(bool pressed)
    {
        static int64_t lastActionTime = esp_timer_get_time();
        actionResponse response;

        if (pressed == true) // button is pressed
        {
            lastActionTime = esp_timer_get_time();
        }
        else
        {
            response.releaseTimeMilliSecs = (esp_timer_get_time() - lastActionTime) / 1000;
        }

        response.command = CMD_SYS_POWER;
        return response;
    }

    // Implement more actions here
}
