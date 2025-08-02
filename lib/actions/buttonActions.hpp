#pragma once
#include "ButtonAction.hpp"
#include "esp_log.h"

namespace actions
{

    class ToggleTrack : public ButtonAction
    {
    public:
        bool execute() override;
    };

    class VolumeUp : public ButtonAction
    {
    public:
        bool execute() override;
    };

    extern ToggleTrack toggleTrackInstance;
    extern VolumeUp volumeUpInstance;
}
