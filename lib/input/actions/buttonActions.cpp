#include "input/actions/buttonActions.hpp"

#include "input/actions/SimpleCommandAction.hpp"

namespace actions
{
    ToggleDac ToggleDacInstance;
    ToggleDisplay ToggleDisplayInstance;
    ToggleMeterDisplay ToggleMeterDisplayInstance;
    PowerButton PowerButtonInstance;
    RotaryEvent RotaryEventInstance;
    CoverView CoverViewInstance;

    static SimpleCommandAction sPreviousTrackCmd(CMD_PREVIOUS_TRACK);
    static SimpleCommandAction sNextTrackCmd(CMD_NEXT_TRACK);
    static SimpleCommandAction sSkipForwardCmd(CMD_SKIP_FORWARD);
    static SimpleCommandAction sSkipBackCmd(CMD_SKIP_BACK);
    static SimpleCommandAction sPlayPauseCmd(CMD_PLAY_PAUSE);
    static SimpleCommandAction sStopCmd(CMD_STOP_TRACK);
    static SimpleCommandAction sPreviousMenuCmd(CMD_PREV_MENU_ITEM);
    static SimpleCommandAction sNextMenuCmd(CMD_NEXT_MENU_ITEM);
    static SimpleCommandAction sMenuSelectCmd(CMD_ITEM_SELECT);
    static SimpleCommandAction sCycleBrightnessCmd(CMD_CYCLE_BRIGHTNESS);

    ButtonAction &PreviousTrackInstance = sPreviousTrackCmd;
    ButtonAction &NextTrackInstance = sNextTrackCmd;
    ButtonAction &SkipForwardInstance = sSkipForwardCmd;
    ButtonAction &SkipBackInstance = sSkipBackCmd;
    ButtonAction &PlayPauseInstance = sPlayPauseCmd;
    ButtonAction &StopInstance = sStopCmd;
    ButtonAction &PreviousMenuInstance = sPreviousMenuCmd;
    ButtonAction &NextMenuInstance = sNextMenuCmd;
    ButtonAction &MenuSelectInstance = sMenuSelectCmd;
    ButtonAction &CycleBrightnessInstance = sCycleBrightnessCmd;
}