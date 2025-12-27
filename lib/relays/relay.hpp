#pragma once
#include <driver/gpio.h>
#include "esp_log.h"

// relay pinout mapping
#define PIN_RELAY_SCREEN_POWER GPIO_NUM_13            // GPIO for screen power relay
#define PIN_RELAY_DAC_POWER GPIO_NUM_12               //  5V and 3.3
#define PIN_RELAY_RPI_POWER GPIO_NUM_11               //  Raspbury Pi
#define PIN_RELAY_OUTPUT_STAGE_POWER GPIO_NUM_9       //  Output stage power relay
#define PIN_RELAY_PROTO_DAC_ENABLED GPIO_NUM_10         //  redirect Audio from ProtoDAC to output 
#define PIN_RELAY_GENERAL_1 GPIO_NUM_47         //  General purpose relay
#define PIN_RELAY_GENERAL_2 GPIO_NUM_39          //  General purpose relay

namespace relays
{
    class StandardRelay
    {
    public:
        // Singleton instance accessor
        static StandardRelay instance() {
            static StandardRelay instance;
            return instance;
    }
        // Initialize the relays
        static void init(gpio_num_t pinRelay);
        static void setRelayState(gpio_num_t relay, bool state);

    private:
        // Add any private members or methods if needed
    };
}