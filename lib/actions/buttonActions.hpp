#pragma once
#include "actionsResponse.hpp"
#include "ActionTemplates.hpp"  // <-- add this

namespace actions
{
    using ToggleDac          = ToggleAction<CMD_TOGGLE_DAC_ON, CMD_TOGGLE_DAC_OFF>;
    using SwitchOffDisplay   = ToggleAction<CMD_DISPLAY_OFF, CMD_DISPLAY_ON>;
    using ToggleMeterDisplay = ToggleAction<CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF>;
    using RotaryRight        = MomentaryAction<CMD_ROTARY_RIGHT>;
    using RotaryLeft         = MomentaryAction<CMD_ROTARY_LEFT>;
    using PowerButton        = TimedAction<CMD_SYS_POWER>;

    extern ToggleDac ToggleDacInstance;
    extern SwitchOffDisplay SwitchOffDisplayInstance;
    extern ToggleMeterDisplay ToggleMeterDisplayInstance;
    extern PowerButton PowerButtonInstance;
    extern RotaryRight RotaryRightInstance;
    extern RotaryLeft RotaryLeftInstance;

    extern ButtonAction& PreviousTrackInstance;
    extern ButtonAction& NextTrackInstance;
    extern ButtonAction& SkipForwardInstance;
    extern ButtonAction& SkipBackInstance;
    extern ButtonAction& PauseInstance;
    extern ButtonAction& StopInstance;
    extern ButtonAction& PreviousMenuInstance;
    extern ButtonAction& NextMenuInstance;
    extern ButtonAction& MenuSelectInstance;
}