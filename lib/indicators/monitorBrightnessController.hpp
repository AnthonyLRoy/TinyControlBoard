#pragma once

#include "driver/ledc.h"
#include "esp_timer.h"
#include "pwmLed.hpp"
#include "PowerStateManager.hpp"
#include "led_definitions.hpp"
#include "esp_log.h"

namespace indicators
{
    class MonitorBrightnessController
    {
    public:
        MonitorBrightnessController(gpio_num_t monitorPin, ledc_channel_t pwmChannel)
            : monitorPin(monitorPin), pwmChannel(pwmChannel)
        {
            // Just store values; do not create timers here
        }
        ~MonitorBrightnessController();

        // Must be called after app_main() starts
        void init();
        bool started = false;
        void setState(ControlBoardPowerState state);
        ControlBoardPowerState getState() const { return currentPowerState; }
        void setBrightness(int brightness);

        void ChangeBrightnessLevel(int change);
        void cycleBrightness();

    private:
        led::LEDPWM monitorLed;
        int BrightnessLevels[10] = {0, 500, 750, 1000, 1250, 1500, 2000, 3000, 3500, 4000};
        int currentBrightnessLevel = 5; // Start at medium brightness

        // LEDs

        ControlBoardPowerState currentPowerState = ControlBoardPowerState::OFF;

        // Store pin/channel info for init
        gpio_num_t monitorPin;
        ledc_channel_t pwmChannel;
    };
}
