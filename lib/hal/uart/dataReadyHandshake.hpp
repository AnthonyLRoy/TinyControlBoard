#pragma once

#include "driver/gpio.h"

namespace transport::uart
{
    // Owns the two data-ready handshake GPIO lines between the ESP32 and the Raspberry Pi:
    // an input the Pi pulses before sending, and an output the ESP32 pulses after sending.
    class DataReadyHandshake
    {
    public:
        bool configureRpiInputPin(gpio_num_t pin);
        bool configureEsp32OutputPin(gpio_num_t esp32DataReadyPin);
        bool installIsrHandler(gpio_num_t rpiDataReadyPin, gpio_isr_t handler, void *p_arg);

        // True while the ESP32's own data-ready output is still asserted from a prior send.
        bool isPulseInProgress() const;

        // Pulses the ESP32 data-ready output high for the handshake hold time, then low.
        void pulseDataReady() const;

    private:
        gpio_num_t m_esp32DataReadyPin = GPIO_NUM_NC;
    };
} // namespace transport::uart
