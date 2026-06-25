#pragma once

#include "protocol/uartProtocol.hpp"

namespace transport::uart
{
    struct IUartCommandSink
    {
        virtual ~IUartCommandSink() = default;
        virtual void sendUartCommand(const char *p_logTag, uint32_t commandId) = 0;
        virtual void sendUartMessage(const char *p_logTag, UartMessage &rMessage) = 0;
    };
}