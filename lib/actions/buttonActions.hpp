#pragma once
#include "actionsResponse.hpp"
#include "ActionTemplates.hpp" // <-- add this

namespace actions
{
    using ToggleDac = ToggleAction<CMD_TOGGLE_DAC_ON, CMD_TOGGLE_DAC_OFF>;
    using ToggleDisplay = ToggleAction<CMD_DISPLAY_OFF, CMD_DISPLAY_ON>;
    using ToggleMeterDisplay = ToggleAction<CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF>;
    using RotaryEvent = RotaryAction<CMD_ROTARY_ACTION>;
    using PowerButton = TimedAction<CMD_SYS_POWER>;
    using CoverView = ToggleAction<CMD_COVER_VIEW_ON, CMD_COVER_VIEW_OFF>;

    extern ToggleDac ToggleDacInstance;
    extern ToggleDisplay ToggleDisplayInstance;
    extern ToggleMeterDisplay ToggleMeterDisplayInstance;
    extern PowerButton PowerButtonInstance;
    extern RotaryEvent RotaryEventInstance;
    extern CoverView CoverViewInstance;

    extern ButtonAction &PreviousTrackInstance;
    extern ButtonAction &NextTrackInstance;
    extern ButtonAction &SkipForwardInstance;
    extern ButtonAction &SkipBackInstance;
    extern ButtonAction &PlayPauseInstance;
    extern ButtonAction &StopInstance;
    extern ButtonAction &PreviousMenuInstance;
    extern ButtonAction &NextMenuInstance;
    extern ButtonAction &MenuSelectInstance;
    extern ButtonAction &CycleBrightnessInstance;
}