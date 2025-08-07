#pragma once
#include "ButtonAction.hpp"
#include "actionsResponse.hpp"
#include "esp_log.h"

namespace actions
{

    class PowerButton : public ButtonAction
    {
    public:
        actionResponse execute(bool pressed) override;
    };

    class ToggleDac : public ButtonAction
    {
    public:
        actionResponse execute(bool pressed) override;
    };

    class SwitchOffDisplay : public ButtonAction
    {
    public:
        actionResponse execute(bool pressed) override;
    };
    class ToggleMeterDisplay : public ButtonAction
    {
    public:
        actionResponse execute(bool pressed) override;
    };

    class RotaryMove : public ButtonAction
    {
    public:
        actionResponse execute(bool pressed) override;
    };

    extern RotaryMove RotaryMoveInstance;
    extern PowerButton PowerButtonInstance;
    extern ToggleDac ToggleDacInstance;
    extern SwitchOffDisplay SwitchOffDisplayInstance;
    extern ToggleMeterDisplay ToggleMeterDisplayInstance;
    extern ButtonAction &PreviousTrackInstance;
    extern ButtonAction &NextTrackInstance;
    extern ButtonAction &SkipForwardInstance;
    extern ButtonAction &SkipBackInstance;
    extern ButtonAction &PauseInstance;
    extern ButtonAction &StopInstance;
    extern ButtonAction &PreviousMenuInstance;
    extern ButtonAction &NextMenuInstance;
    extern ButtonAction &MenuSelectInstance;

}
