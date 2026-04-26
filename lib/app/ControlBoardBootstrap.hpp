#pragma once

#include "activityStatus.hpp"
#include "input/buttons/mcpInputHandler.hpp"
#include "powerLed.hpp"
#include "relay.hpp"
#include "transport/uart/serial.hpp"
#include <functional>

namespace controlSystem
{
    class ControlBoardBootstrap
    {
    public:
        using SerialRxCallback = std::function<void(const UartMessage &)>;
        using VoidCallback = std::function<void()>;
        using ButtonCallback = std::function<void(uint8_t)>;
        using RotaryCallback = std::function<void(int)>;

        void prepareStartupIndicators() const;
        void finalizeStartupIndicators() const;

        void configureSerialCallbacks(transport::uart::UartTransport &rSerial,
                                      SerialRxCallback onSerialRx,
                                      VoidCallback onHeartbeatTimeout) const;

        bool setupRelays() const;
        bool setupSerial(transport::uart::UartTransport &rSerial) const;
        bool setupMcpHandler(buttons::McpInputHandler &rMcpHandler) const;

        void configureMcpCallbacks(buttons::McpInputHandler &rMcpHandler,
                                   ButtonCallback onButtonPressed,
                                   ButtonCallback onButtonReleased,
                                   RotaryCallback onRotaryMovement) const;
    };
}