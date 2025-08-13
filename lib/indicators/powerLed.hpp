#pragma once
#include "driver/ledc.h"
#include "pwmLed.hpp" // Ensure this header defines the LED class
#include "led_definitions.hpp"

namespace indicators
{
    class PowerLed
    {
    public:
        PowerLed(gpio_num_t PIN_APP_ACTIVE_LED, gpio_num_t PIN_APP_STANDBY_LED,ledc_channel_t channel);
        ~PowerLed();

        // Set the power LED state
        void setState(ControlBoardState state);
        void setBrightness(int brightness);

        // Update the power LED state
        void update();

    private:
        led::LEDPWM onLed;  // LED for power on indication
        led::LEDPWM standBy; // LED for power off indication
        int dutyCycle = 4096; // 50% duty cycle for 13-bit resolution
    };
}