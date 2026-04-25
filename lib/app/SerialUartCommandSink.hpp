#pragma once

#include "app/ActionUartDispatcher.hpp"

namespace serialBus
{
    class Serial;
}

namespace controlSystem
{
    class SerialUartCommandSink : public IUartCommandSink
    {
    public:
        explicit SerialUartCommandSink(serialBus::Serial &rSerial);

        void sendUartCommand(const char *pLogTag, uint32_t commandId) override;
        void sendUartMessage(const char *pLogTag, UartMessage &rMessage) override;

    private:
        serialBus::Serial &mrSerial;
    };
}