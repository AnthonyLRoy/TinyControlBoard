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
        actionResponse execute(bool buttonMode) override;
    };

    class VolumeUp : public ButtonAction
    {
    public:
        actionResponse execute(bool buttonMode) override;
    };
  class PowerButton : public ButtonAction
    {
    public:
        actionResponse execute(bool buttonMode) override;
    };

    extern PowerButton PowerButtonInstance;
    extern ToggleTrack toggleTrackInstance;
    extern VolumeUp volumeUpInstance;
}
