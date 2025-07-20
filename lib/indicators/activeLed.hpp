
#pragma once
#include "driver/ledc.h"
#include "pwmLed.hpp" // Ensure this header defines the LED class
#include "led_definitions.hpp"


namespace indicators
{
    class ActiveLed
    {
    public:
        ActiveLed(gpio_num_t PIN_APP_ACTIVE_LED);
        ~ActiveLed();

        void SetStatus(ControlBoardWorkingStatus newStatus);

        // Update the active LED state
        void update();

    private:
        led::LEDPWM activeLed; // LED for active indication
    };
}