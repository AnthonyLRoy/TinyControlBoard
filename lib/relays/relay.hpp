#pragma once
#include <driver/gpio.h>
#include "board/boardConfig.hpp"
#include "esp_log.h"

// relay pinout mapping
inline constexpr gpio_num_t PIN_RELAY_SCREEN_POWER = board::relays::kScreenPower;
inline constexpr gpio_num_t PIN_RELAY_DAC_POWER = board::relays::kDacPower;
inline constexpr gpio_num_t PIN_RELAY_RPI_POWER = board::relays::kRpiPower;
inline constexpr gpio_num_t PIN_RELAY_OUTPUT_STAGE_POWER = board::relays::kOutputStagePower;
inline constexpr gpio_num_t PIN_RELAY_PROTO_DAC_ENABLED = board::relays::kProtoDacEnabled;
inline constexpr gpio_num_t PIN_RELAY_GENERAL_1 = board::relays::kGeneral1;
inline constexpr gpio_num_t PIN_RELAY_GENERAL_2 = board::relays::kGeneral2;

namespace relays
{
    class StandardRelay
    {
    public:
        // Singleton instance accessor
        static StandardRelay &getInstance() {
            static StandardRelay sInstance;
            return sInstance;
        }
        // Initialize the relays
        static void init(gpio_num_t pinRelay);
        static void setRelayState(gpio_num_t relay, bool state);

    private:
        // Add any private members or methods if needed
    };
}