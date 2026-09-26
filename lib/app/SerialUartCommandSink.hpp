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
        // Creates the sink bound to the serial transport used to emit board commands.
        explicit SerialUartCommandSink(transport::uart::UartTransport &rSerial);

        // Sends a bare command ID over the UART transport with the supplied log tag.
        void sendUartCommand(const char *p_logTag, uint32_t commandId) override;
        // Serializes and emits a complete UART message over the board transport.
        void sendUartMessage(const char *p_logTag, UartMessage &rMessage) override;

    private:
        transport::uart::UartTransport &mr_serial;
    };
}