#pragma once
#include "uart_protocol.hpp"
#include "ButtonAction.hpp"
#include "actionsResponse.hpp"
#include "esp_log.h"

namespace actions
{

    class ToggleTrack : public ButtonAction
    {
    public:
        actionResponse execute() override;
    };

    class VolumeUp : public ButtonAction
    {
    public:
        actionResponse execute() override;
    };

    extern ToggleTrack toggleTrackInstance;
    extern VolumeUp volumeUpInstance;
}
