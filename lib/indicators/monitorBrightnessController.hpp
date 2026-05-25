#pragma once

#include "driver/ledc.h"
#include "esp_timer.h"
#include "power/powerState.hpp"
#include "hal/leds/pwmLed.hpp"
#include "indicators/ledDefinitions.hpp"
#include "esp_log.h"
#include "hal/storage/nvsStorage.hpp"

namespace indicators
{
    class MonitorBrightnessController
    {
    public:
        MonitorBrightnessController(gpio_num_t monitorPin, ledc_channel_t pwmChannel)
            : m_monitorPin(monitorPin), m_pwmChannel(pwmChannel)
        {
            // Just store values; do not create timers here
        }
        ~MonitorBrightnessController();

        // Must be called after app_main() starts
        void init();
        bool m_started = false;
        void setState(ControlBoardPowerState state);
        ControlBoardPowerState getState() const { return m_currentPowerState; }
        void setBrightness(int brightness);
        void setBlanked(bool blanked);
        bool isBlanked() const { return m_blanked; }

        void changeBrightnessLevel(int change);
        void cycleBrightness();

    private:
        support::NvsStorage m_nvsStorage{"brightness"};
        led::LedPwm m_monitorLed;
        int m_brightnessLevels[12] = {10, 109, 568, 1127, 1486, 1845, 2205, 2564, 2923, 3282, 3641, 4000};

        int m_currentBrightnessLevel = 2; // Start at medium brightness
        int m_savedBrightnessLevel = 2;   // Saved before sleep/off, restored on wake/on
        bool m_blanked = false;

        ControlBoardPowerState m_currentPowerState = ControlBoardPowerState::OFF;

        // Store pin/channel info for init
        gpio_num_t m_monitorPin;
        ledc_channel_t m_pwmChannel;
    };
}
