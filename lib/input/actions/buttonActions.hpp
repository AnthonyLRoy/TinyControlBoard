#pragma once

#include "input/actions/actionsResponse.hpp"
#include "input/actions/actionTemplates.hpp"
#include "input/actions/actionTemplates.hpp"

namespace actions
{
    using ToggleDac = ToggleAction<CMD_TOGGLE_DAC_ON, CMD_TOGGLE_DAC_OFF>;
    using ToggleMeterDisplay = ToggleAction<CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF>;
    using CoverView = ToggleAction<CMD_COVER_VIEW_ON, CMD_COVER_VIEW_OFF>;
    using ToggleRepeat = ToggleAction<CMD_REPEAT_ON, CMD_REPEAT_OFF>;
    using ToggleRandom = ToggleAction<CMD_RANDOM_ON, CMD_RANDOM_OFF>;
    using RotaryEvent = RotaryAction<CMD_ROTARY_ACTION>;
    using PowerButton = TimedAction<CMD_SYS_POWER>;

    extern ToggleDac ToggleDacInstance;
    extern ToggleMeterDisplay ToggleMeterDisplayInstance;
    extern CoverView CoverViewInstance;
    extern ToggleRepeat RepeatInstance;
    extern ToggleRandom ToggleRandomInstance;
    extern RotaryEvent RotaryEventInstance;
    extern PowerButton PowerButtonInstance;

    extern SimpleCommandAction PreviousTrackInstance;
    extern SimpleCommandAction NextTrackInstance;
    extern SimpleCommandAction SkipForwardInstance;
    extern SimpleCommandAction SkipBackInstance;
    extern SimpleCommandAction PlayPauseInstance;
    extern SimpleCommandAction DisplayOffOnInstance;
    extern SimpleCommandAction PreviousMenuInstance;
    extern SimpleCommandAction NextMenuInstance;
    extern SimpleCommandAction MenuSelectInstance;
    extern SimpleCommandAction CycleBrightnessInstance;
}