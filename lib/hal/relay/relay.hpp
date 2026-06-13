#pragma once
#include <driver/gpio.h>
#include "board/boardConfig.hpp"
#include "esp_log.h"

namespace relays
{
    class StandardRelay
    {
    public:
        // Singleton instance accessor
        static StandardRelay &getInstance() {
            static StandardRelay s_instance;
            return s_instance;
        }
        // Initialize the relays
        static void init(gpio_num_t pinRelay);
        static void setRelayState(gpio_num_t relay, bool state);

    private:
        // Add any private members or methods if needed
    };
}