#include "relay.hpp"

namespace relays
{



void StandardRelay::init(gpio_num_t pinRelay)
{
    // Initialize the GPIO pin for the relay
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    io_conf.mode = GPIO_MODE_OUTPUT;       // Set as output mode
    io_conf.pin_bit_mask = (1ULL << pinRelay); // Set the pin bit mask
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     // Disable pull-up

    // Apply the configuration
    gpio_config(&io_conf);
}

void StandardRelay::setRelayState(gpio_num_t relayPin, bool state)
{
    // Set the GPIO state based on the relay state
    gpio_set_level(relayPin, state ? 1 : 0);
}

}