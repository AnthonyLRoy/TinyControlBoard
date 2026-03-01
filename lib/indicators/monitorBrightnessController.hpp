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
            : mMonitorPin(monitorPin), mPwmChannel(pwmChannel)
        {
            // Just store values; do not create timers here
        }
        ~MonitorBrightnessController();

        // Must be called after app_main() starts
        void init();
        bool mStarted = false;
        void setState(ControlBoardPowerState state);
        ControlBoardPowerState getState() const { return mCurrentPowerState; }
        void setBrightness(int brightness);

        void changeBrightnessLevel(int change);
        void cycleBrightness();

    private:
        led::LedPwm mMonitorLed;
        int mBrightnessLevels[10] = {0, 500, 750, 1000, 1250, 1500, 2000, 3000, 3500, 4000};
        int mCurrentBrightnessLevel = 5; // Start at medium brightness

        // LEDs

        ControlBoardPowerState mCurrentPowerState = ControlBoardPowerState::OFF;

        // Store pin/channel info for init
        gpio_num_t mMonitorPin;
        ledc_channel_t mPwmChannel;
    };
}
