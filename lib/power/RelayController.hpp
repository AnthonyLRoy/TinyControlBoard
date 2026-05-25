#pragma once

#include "hal/relay/relay.hpp"
#include "hal/uart/serial.hpp"
#include <driver/gpio.h>
#include <cstdint>

namespace controlSystem
{
    class RelayController
    {
    public:
        RelayController(transport::uart::UartTransport &rSerial, relays::StandardRelay &rRelays);

        void setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs);
        bool handleToggleDac(bool state);
        bool shutdownRpi(bool wait);
        bool shutdownScreen(bool wait);

    private:
        transport::uart::UartTransport &mr_serial;
        relays::StandardRelay &mr_relays;
        static constexpr const char *k_logTag = "Relay_Controller";
    };
}