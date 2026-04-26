#pragma once

#include "app/ActionUartDispatcher.hpp"

namespace transport::uart
{
    class UartTransport;
}

namespace controlSystem
{
    class SerialUartCommandSink : public IUartCommandSink
    {
    public:
        explicit SerialUartCommandSink(transport::uart::UartTransport &rSerial);

        void sendUartCommand(const char *pLogTag, uint32_t commandId) override;
        void sendUartMessage(const char *pLogTag, UartMessage &rMessage) override;

    private:
        transport::uart::UartTransport &mrSerial;
    };
}