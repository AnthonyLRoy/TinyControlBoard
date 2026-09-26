#pragma once

#include "driver/ledc.h"
#include "esp_timer.h"
#include "power/powerState.hpp"
#include "hal/leds/pwmLed.hpp"
#include "indicators/ledDefinitions.hpp"
#include "esp_log.h"

namespace indicators
{
    class PowerLed
    {
    public:
        // Stores the active and standby LED pins/channels without creating timers.
        PowerLed(gpio_num_t activePin,
                 ledc_channel_t activeChannel,
                 gpio_num_t standbyPin,
                 ledc_channel_t standbyChannel);

        // Stops and releases the periodic timer that updates LED patterns.
        ~PowerLed();

        // Initializes both PWM outputs and starts the periodic pattern-update timer.
        void init();
        bool m_started = false;
        // Selects the active/standby LED pattern associated with a power state.
        void setState(ControlBoardPowerState state);
        // Returns the current board power state displayed by this controller.
        ControlBoardPowerState getState() const { return m_currentPowerState; }
        // Sets the brightness used by the active LED's on state, clamped to the supported range.
        void setBrightness(int brightness);

    private:
        // Computes and applies the current flashing, breathing, or blip pattern.
        void update();
        // Timer callback that advances the power LED pattern.
        static void handleTimer(void *p_arg);

        // LEDs
        led::LedPwm m_activeLed;
        led::LedPwm m_standbyLed;

        // Timer handle (created in init)
        esp_timer_handle_t mp_updateTimer = nullptr;

        // Power state
        ControlBoardPowerState m_currentPowerState = ControlBoardPowerState::OFF;

        // Flashing
        bool m_activeFlash = false;
        bool m_standbyFlash = false;
        bool m_flashState = false;
        uint64_t m_lastFlashToggle = 0;

        // Breathing effect
        bool m_activeBreathing = false;
        bool m_standbyBreathing = false;
        uint64_t m_breathingStartTime = 0;
        const uint32_t BREATHING_PERIOD = 4000; // 2 seconds for full cycle

        // Blip effect (short pulse)
        bool m_activeBlip = false;
        bool m_standbyBlip = false;
        uint64_t m_blipStartTime = 0;
        const uint32_t BLIP_DURATION = 125;    // 1/8 second
        const uint32_t BLIP_PERIOD = 10000;    // every 10 seconds

        // LED duty levels
        int m_dutyCycle = 4096;
        int m_mediumDuty = 2048;
        int m_offDuty = 0;

        // Store pin/channel info for init
        gpio_num_t m_activePin;
        ledc_channel_t m_activeChannel;
        gpio_num_t m_standbyPin;
        ledc_channel_t m_standbyChannel;
    };
}
