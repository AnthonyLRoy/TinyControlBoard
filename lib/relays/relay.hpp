#pragma once
#include <driver/gpio.h>
#include "esp_log.h"
#define PIN_RELAY_SCREEN GPIO_NUM_13 // GPIO for screen power relay
#define PIN_RELAY_DAC GPIO_NUM_12    //  5V and 3.3
#define PIN_RELAY_MAINS GPIO_NUM_11  //  Raspbury Pi
#define PIN_RELAY_GENERAL_2 GPIO_NUM_10
#define PIN_RELAY_GENERAL_3 GPIO_NUM_9  //  General purpose relay
#define PIN_RELAY_GENERAL_4 GPIO_NUM_47 //  General purpose relay

namespace relays
{



    class StandardRelay
    {
    public:
        // Initialize the relays
        static void init(gpio_num_t pinRelay);
         static void setRelayState(gpio_num_t relay, bool state);

    private:
        // Add any private members or methods if needed
    };
}