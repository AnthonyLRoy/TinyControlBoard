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
        bool mStarted = false;
        void setState(ControlBoardPowerState state);
        ControlBoardPowerState getState() const { return mCurrentPowerState; }
        void setBrightness(int brightness);

    private:
        void update();
        static void handleTimer(void *pArg);

        // LEDs
        led::LedPwm mActiveLed;
        led::LedPwm mStandbyLed;

        // Timer handle (created in init)
        esp_timer_handle_t mpUpdateTimer = nullptr;

        // Power state
        ControlBoardPowerState mCurrentPowerState = ControlBoardPowerState::OFF;

        // Flashing
        bool mActiveFlash = false;
        bool mStandbyFlash = false;
        bool mFlashState = false;
        uint64_t mLastFlashToggle = 0;

        // Breathing effect
        bool mActiveBreathing = false;
        bool mStandbyBreathing = false;
        uint64_t mBreathingStartTime = 0;
        const uint32_t BREATHING_PERIOD = 4000; // 2 seconds for full cycle

        // Blip effect (short pulse)
        bool mActiveBlip = false;
        bool mStandbyBlip = false;
        uint64_t mBlipStartTime = 0;
        const uint32_t BLIP_DURATION = 125;    // 1/8 second
        const uint32_t BLIP_PERIOD = 10000;    // every 10 seconds

        // LED duty levels
        int mDutyCycle = 4096;
        int mMediumDuty = 2048;
        int mOffDuty = 0;

        // Store pin/channel info for init
        gpio_num_t mActivePin;
        ledc_channel_t mActiveChannel;
        gpio_num_t mStandbyPin;
        ledc_channel_t mStandbyChannel;
    };
}
