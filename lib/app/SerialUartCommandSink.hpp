#pragma once

#include "app/ActionUartDispatcher.hpp"

namespace transport::uart
{
    class Serial;
}

namespace controlSystem
{
    class SerialUartCommandSink : public IUartCommandSink
    {
    public:
        explicit SerialUartCommandSink(transport::uart::Serial &rSerial);

        void sendUartCommand(const char *pLogTag, uint32_t commandId) override;
        void sendUartMessage(const char *pLogTag, UartMessage &rMessage) override;

    private:
        transport::uart::Serial &mrSerial;
    };
}