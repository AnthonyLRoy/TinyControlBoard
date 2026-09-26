#pragma once

#include "indicators/activityStatus.hpp"
#include "hal/buttons/mcpInputHandler.hpp"
#include "indicators/powerLed.hpp"
#include "hal/relay/relay.hpp"
#include "hal/uart/serial.hpp"
#include <functional>

namespace controlSystem
{
    namespace bootstrap
    {
        using SerialRxCallback = std::function<void(const UartMessage &)>;
        using VoidCallback = std::function<void()>;
        using ButtonCallback = std::function<void(uint8_t)>;
        using RotaryCallback = std::function<void(int)>;

        // Initializes the board LEDs and status indicators needed during startup.
        void prepareStartupIndicators();
        // Finalizes the startup indicator pattern once the system is ready.
        void finalizeStartupIndicators();

        // Hooks the UART transport callbacks used for incoming messages and heartbeat failures.
        void configureSerialCallbacks(transport::uart::UartTransport &rSerial,
                                      SerialRxCallback onSerialRx,
                                      VoidCallback onHeartbeatTimeout);

        // Powers on and initializes the relay hardware for the board subsystems.
        bool setupRelays();
        // Configures the serial transport used for Raspberry Pi communication.
        bool setupSerial(transport::uart::UartTransport &rSerial);
        // Configures the MCP GPIO expander used to read the physical panel inputs.
        bool setupMcpHandler(buttons::McpInputHandler &rMcpHandler);

        // Wires the MCP callback handlers for button presses, releases, and rotary movement.
        void configureMcpCallbacks(buttons::McpInputHandler &rMcpHandler,
                                   ButtonCallback onButtonPressed,
                                   ButtonCallback onButtonReleased,
                                   RotaryCallback onRotaryMovement);
    }
}