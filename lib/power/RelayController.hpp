#pragma once

#include "relay.hpp"
#include "transport/uart/serial.hpp"
#include <driver/gpio.h>
#include <cstdint>

namespace controlSystem
{
    class RelayController
    {
    public:
        RelayController(transport::uart::Serial &rSerial, relays::StandardRelay &rRelays);

        void setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs);
        bool handleToggleDac(bool state);
        bool shutdownRpi(bool wait);
        bool shutdownScreen(bool wait);

    private:
        transport::uart::Serial &mrSerial;
        relays::StandardRelay &mrRelays;
        static constexpr const char *mspTag = "RelayController";
    };
}