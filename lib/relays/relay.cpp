#include "relay.hpp"

namespace relays
{

static const char *TAG = "RELAY";

void StandardRelay::init(gpio_num_t pinRelay)
{
    ESP_LOGI(TAG, "Initializing relay on GPIO %d", pinRelay);
    // Initialize the GPIO pin for the relay
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    io_conf.mode = GPIO_MODE_OUTPUT;       // Set as output mode
    io_conf.pin_bit_mask = (1ULL << pinRelay); // Set the pin bit mask
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // Disable pull-down
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     // Disable pull-up

    // Apply the configuration
    gpio_config(&io_conf);
}

void StandardRelay::setRelayState(gpio_num_t relayPin, bool state)
{
    ESP_LOGI(TAG, "Setting relay on GPIO %d to %s", relayPin, state ? "ON" : "OFF");
    // Set the GPIO state based on the relay state
    gpio_set_level(relayPin, state ? 1 : 0);
}

}