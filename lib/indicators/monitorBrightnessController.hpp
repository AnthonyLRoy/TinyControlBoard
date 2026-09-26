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
        // Stores the monitor backlight pin and PWM channel for deferred initialization.
        MonitorBrightnessController(gpio_num_t monitorPin, ledc_channel_t pwmChannel)
            : m_monitorPin(monitorPin), m_pwmChannel(pwmChannel)
        {
            // Just store values; do not create timers here
        }
        // Releases any resources owned by the brightness controller.
        ~MonitorBrightnessController();

        // Initializes the monitor PWM output and restores the saved brightness from NVS.
        void init();
        bool m_started = false;
        // Updates the controller's response to a board power-state transition.
        void setState(ControlBoardPowerState state);
        // Returns the most recently applied board power state.
        ControlBoardPowerState getState() const { return m_currentPowerState; }
        // Sets and persists the monitor brightness level.
        void setBrightness(int brightness);
        // Raises brightness by the requested number of levels.
        void setBrightnessUp(int levels = 1);
        // Lowers brightness by the requested number of levels.
        void setBrightnessDown(int levels = 1);
        // Blanks or restores the monitor backlight without changing the selected brightness level.
        void setBlanked(bool blanked);
        // Reports whether the monitor backlight is currently blanked.
        bool isBlanked() const { return m_blanked; }
        // Toggles display-off mode, saving or restoring the selected brightness.
        void toggleDisplayOffOn();
        // Clears display-off mode and restores the saved brightness before a power transition.
        void clearDisplayOffMode();
        // Reports whether the display-off toggle mode is active.
        bool isDisplayOffActive() const { return m_displayOffActive; }

        // Applies a signed brightness-level change within the supported range.
        void changeBrightnessLevel(int change);
        // Advances to the next brightness level, wrapping to the first level.
        void cycleBrightness();

    private:
        support::NvsStorage m_nvsStorage{"brightness"};
        led::LedPwm m_monitorLed;
        int m_brightnessLevels[12] = {10, 109, 568, 1127, 1486, 1845, 2205, 2564, 2923, 3282, 3641, 4000};

        int m_currentBrightnessLevel = 2; // Start at medium brightness
        int m_savedBrightnessLevel = 2;   // Saved before sleep/off, restored on wake/on
        int m_displayOffSavedBrightnessLevel = 2;
        bool m_blanked = false;
        bool m_displayOffActive = false;

        ControlBoardPowerState m_currentPowerState = ControlBoardPowerState::OFF;

        // Store pin/channel info for init
        gpio_num_t m_monitorPin;
        ledc_channel_t m_pwmChannel;
    };
}
