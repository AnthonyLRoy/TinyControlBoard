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
        RelayController(serialBus::Serial &rSerial, relays::StandardRelay &rRelays);

        // Set a relay with optional delay
        void setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs);

        // Handle DAC toggle
        bool handleToggleDac(bool state);

        // Shutdown RPI and optionally wait for confirmation
        bool shutdownRpi(bool wait);

        // Shutdown screen
        bool shutdownScreen(bool wait);

    private:
        serialBus::Serial &mrSerial;
        relays::StandardRelay &mrRelays;
        static constexpr const char *mspTag = "RelayController";
    };
}
