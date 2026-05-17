#pragma once

#include "driver/ledc.h"
#include "esp_timer.h"
#include "power/powerState.hpp"
#include "indicators/pwm/pwmLed.hpp"
#include "ledDefinitions.hpp"
#include "esp_log.h"
#include "support/nvsStorage.hpp"

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
        void setBlanked(bool blanked);
        bool isBlanked() const { return mBlanked; }

        void changeBrightnessLevel(int change);
        void cycleBrightness();

    private:
        support::NvsStorage mNvsStorage{"brightness"};
        led::LedPwm mMonitorLed;
        int mBrightnessLevels[12] = {10, 109, 568, 1127, 1486, 1845, 2205, 2564, 2923, 3282, 3641, 4000};

        int mCurrentBrightnessLevel = 2; // Start at medium brightness
        int mSavedBrightnessLevel = 2;   // Saved before sleep/off, restored on wake/on
        bool mBlanked = false;

        ControlBoardPowerState mCurrentPowerState = ControlBoardPowerState::OFF;

        // Store pin/channel info for init
        gpio_num_t mMonitorPin;
        ledc_channel_t mPwmChannel;
    };
}
