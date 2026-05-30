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

        void prepareStartupIndicators();
        void finalizeStartupIndicators();

        void configureSerialCallbacks(transport::uart::UartTransport &rSerial,
                                      SerialRxCallback onSerialRx,
                                      VoidCallback onHeartbeatTimeout);

        bool setupRelays();
        bool setupSerial(transport::uart::UartTransport &rSerial);
        bool setupMcpHandler(buttons::McpInputHandler &rMcpHandler);

        void configureMcpCallbacks(buttons::McpInputHandler &rMcpHandler,
                                   ButtonCallback onButtonPressed,
                                   ButtonCallback onButtonReleased,
                                   RotaryCallback onRotaryMovement);
    }
}