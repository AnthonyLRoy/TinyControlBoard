#pragma once

#include "driver/ledc.h"
#include "esp_timer.h"
#include "pwmLed.hpp"
#include "PowerStateManager.hpp"
#include "led_definitions.hpp"
#include "esp_log.h"

namespace indicators
{
    class PowerLed
    {
    public:
        // Constructor only stores pins/channels, no timers are created
        PowerLed(gpio_num_t activePin,
                 ledc_channel_t activeChannel,
                 gpio_num_t standbyPin,
                 ledc_channel_t standbyChannel);

        ~PowerLed();

        // Must be called after app_main() starts
        void init();
        bool started = false;
        void setState(ControlBoardPowerState state);
        ControlBoardPowerState getState() const { return currentPowerState; }
        void setBrightness(int brightness);

    private:
        void update();
        static void timerCallback(void *arg);

        // LEDs
        led::LEDPWM activeLed;
        led::LEDPWM standbyLed;

        // Timer handle (created in init)
        esp_timer_handle_t updateTimer = nullptr;

        // Power state
        ControlBoardPowerState currentPowerState = ControlBoardPowerState::OFF;

        // Flashing
        bool activeFlash = false;
        bool standbyFlash = false;
        bool flashState = false;
        uint64_t lastFlashToggle = 0;

        // Breathing effect
        bool activeBreathing = false;
        bool standbyBreathing = false;
        uint64_t breathingStartTime = 0;
        const uint32_t BREATHING_PERIOD = 4000; // 2 seconds for full cycle

        // Blip effect (short pulse)
        bool activeBlip = false;
        bool standbyBlip = false;
        uint64_t blipStartTime = 0;
        const uint32_t BLIP_DURATION = 125;    // 1/8 second
        const uint32_t BLIP_PERIOD = 10000;    // every 10 seconds

        // LED duty levels
        int dutyCycle = 4096;
        int mediumDuty = 2048;
        int offDuty = 0;

        // Store pin/channel info for init
        gpio_num_t activePin;
        ledc_channel_t activeChannel;
        gpio_num_t standbyPin;
        ledc_channel_t standbyChannel;
    };
}
