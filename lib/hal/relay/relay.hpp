#pragma once
#include <driver/gpio.h>
#include "board/boardConfig.hpp"
#include "esp_log.h"

// relay pinout mapping
inline constexpr gpio_num_t PIN_RELAY_SCREEN_POWER = board::relays::k_screenPower;
inline constexpr gpio_num_t PIN_RELAY_DAC_POWER = board::relays::k_dacPower;
inline constexpr gpio_num_t PIN_RELAY_RPI_POWER = board::relays::k_rpiPower;
inline constexpr gpio_num_t PIN_RELAY_OUTPUT_STAGE_POWER = board::relays::k_outputStagePower;
inline constexpr gpio_num_t PIN_RELAY_ESS_DAC_ENABLED = board::relays::k_essDacEnabled;
inline constexpr gpio_num_t PIN_RELAY_GENERAL_1 = board::relays::k_general1;
inline constexpr gpio_num_t PIN_RELAY_GENERAL_2 = board::relays::k_general2;

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