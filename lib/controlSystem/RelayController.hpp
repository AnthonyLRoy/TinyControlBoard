#pragma once

#include "relay.hpp"
#include "serial.hpp"
#include <driver/gpio.h>
#include <cstdint>

namespace controlSystem
{
    /**
     * Manages relay control operations including DAC control, RPI shutdown, and screen shutdown.
     * Encapsulates relay timing and UART command integration.
     */
    class RelayController
    {
    public:
        RelayController(serialBus::Serial &serialRef, relays::StandardRelay &relaysRef);

        // Set a relay with optional delay
        void setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs);

        // Handle DAC toggle
        bool HandleToggleDac(bool state);

        // Shutdown RPI and optionally wait for confirmation
        bool ShutDownRPI(bool wait);

        // Shutdown screen
        bool ShutDownScreen(bool wait);

    private:
        serialBus::Serial &serial;
        relays::StandardRelay &relays;
        static constexpr const char *TAG = "RelayController";
    };
}
