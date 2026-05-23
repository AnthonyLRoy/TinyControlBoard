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

        void sendUartCommand(const char *p_logTag, uint32_t commandId) override;
        void sendUartMessage(const char *p_logTag, UartMessage &rMessage) override;

    private:
        transport::uart::UartTransport &mr_serial;
    };
}