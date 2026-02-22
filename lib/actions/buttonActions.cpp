#include "buttonActions.hpp"
#include "SimpleCommandAction.hpp"

namespace actions
{
    ToggleDac ToggleDacInstance;
    ToggleDisplay ToggleDisplayInstance;
    ToggleMeterDisplay ToggleMeterDisplayInstance;
    PowerButton PowerButtonInstance;
    RotaryEvent RotaryEventInstance;
    CoverView CoverViewInstance;

    static SimpleCommandAction PreviousTrackCmd(CMD_PREVIOUS_TRACK);
    static SimpleCommandAction NextTrackCmd(CMD_NEXT_TRACK);
    static SimpleCommandAction SkipForwardCmd(CMD_SKIP_FORWARD);
    static SimpleCommandAction SkipBackCmd(CMD_SKIP_BACK);
    static SimpleCommandAction PlayPauseCmd(CMD_PLAY_PAUSE);
    static SimpleCommandAction StopCmd(CMD_STOP_TRACK);
    static SimpleCommandAction PreviousMenuCmd(CMD_PREV_MENU_ITEM);
    static SimpleCommandAction NextMenuCmd(CMD_NEXT_MENU_ITEM);
    static SimpleCommandAction MenuSelectCmd(CMD_ITEM_SELECT);
    static SimpleCommandAction CycleBrightnessCmd(CMD_CYCLE_BRIGHTNESS);

    ButtonAction& PreviousTrackInstance = PreviousTrackCmd;
    ButtonAction& NextTrackInstance     = NextTrackCmd;
    ButtonAction& SkipForwardInstance   = SkipForwardCmd;
    ButtonAction& SkipBackInstance      = SkipBackCmd;
    ButtonAction& PlayPauseInstance     = PlayPauseCmd;
    ButtonAction& StopInstance          = StopCmd;
    ButtonAction& PreviousMenuInstance  = PreviousMenuCmd;
    ButtonAction& NextMenuInstance      = NextMenuCmd;
    ButtonAction& MenuSelectInstance    = MenuSelectCmd;
    ButtonAction& CycleBrightnessInstance = CycleBrightnessCmd;     
}
