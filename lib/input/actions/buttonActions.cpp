#include "input/actions/buttonActions.hpp"

namespace actions
{
    ToggleDac ToggleDacInstance;
    ToggleMeterDisplay ToggleMeterDisplayInstance;
    CoverView CoverViewInstance;
    ToggleRepeat RepeatInstance;
    ToggleRandom ToggleRandomInstance;
    RotaryEvent RotaryEventInstance;
    PowerButton PowerButtonInstance;

    SimpleCommandAction PreviousTrackInstance(CMD_PREVIOUS_TRACK);
    SimpleCommandAction NextTrackInstance(CMD_NEXT_TRACK);
    SimpleCommandAction SkipForwardInstance(CMD_SKIP_FORWARD);
    SimpleCommandAction SkipBackInstance(CMD_SKIP_BACK);
    SimpleCommandAction PlayPauseInstance(CMD_PLAY_PAUSE);
    SimpleCommandAction StopInstance(CMD_STOP_TRACK);
    SimpleCommandAction PreviousMenuInstance(CMD_PREV_MENU_ITEM);
    SimpleCommandAction NextMenuInstance(CMD_NEXT_MENU_ITEM);
    SimpleCommandAction MenuSelectInstance(CMD_ITEM_SELECT);
    SimpleCommandAction CycleBrightnessInstance(CMD_CYCLE_BRIGHTNESS);
}