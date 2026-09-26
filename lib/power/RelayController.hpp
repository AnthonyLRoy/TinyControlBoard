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
        // Creates the relay controller bound to the board serial transport and relay hardware.
        RelayController(transport::uart::UartTransport &rSerial, relays::StandardRelay &rRelays);

        // Sets a relay output after the requested delay, allowing power sequencing to happen safely.
        void setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs);
        // Handles DAC power toggling and returns whether the operation changed the enabled state.
        bool handleToggleDac(bool state);
        // Toggles the DAC power rail and reports the new on/off state.
        bool toggleDac();
        // Shuts down the Raspberry Pi by sequencing the board's power relay state.
        bool shutdownRpi();

    private:
        transport::uart::UartTransport &mr_serial;
        relays::StandardRelay &mr_relays;
        bool m_dacEnabled{false};
        static constexpr const char *k_logTag = "Relay_Controller";
    };
}