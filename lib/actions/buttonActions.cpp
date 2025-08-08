#include "buttonActions.hpp"
#include "SimpleCommandAction.hpp"

namespace actions
{
    ToggleDac ToggleDacInstance;
    SwitchOffDisplay SwitchOffDisplayInstance;
    ToggleMeterDisplay ToggleMeterDisplayInstance;
    PowerButton PowerButtonInstance;
    RotaryRight RotaryRightInstance;
    RotaryLeft RotaryLeftInstance;

    static SimpleCommandAction PreviousTrackCmd(CMD_PREVIOUS_TRACK);
    static SimpleCommandAction NextTrackCmd(CMD_NEXT_TRACK);
    static SimpleCommandAction SkipForwardCmd(CMD_SKIP_FORWARD);
    static SimpleCommandAction SkipBackCmd(CMD_SKIP_BACK);
    static SimpleCommandAction PauseCmd(CMD_PLAY_PAUSE);
    static SimpleCommandAction StopCmd(CMD_STOP_TRACK);
    static SimpleCommandAction PreviousMenuCmd(CMD_PREV_MENU_ITEM);
    static SimpleCommandAction NextMenuCmd(CMD_NEXT_MENU_ITEM);
    static SimpleCommandAction MenuSelectCmd(CMD_ITEM_SELECT);

    ButtonAction& PreviousTrackInstance = PreviousTrackCmd;
    ButtonAction& NextTrackInstance     = NextTrackCmd;
    ButtonAction& SkipForwardInstance   = SkipForwardCmd;
    ButtonAction& SkipBackInstance      = SkipBackCmd;
    ButtonAction& PauseInstance         = PauseCmd;
    ButtonAction& StopInstance          = StopCmd;
    ButtonAction& PreviousMenuInstance  = PreviousMenuCmd;
    ButtonAction& NextMenuInstance      = NextMenuCmd;
    ButtonAction& MenuSelectInstance    = MenuSelectCmd;
}
