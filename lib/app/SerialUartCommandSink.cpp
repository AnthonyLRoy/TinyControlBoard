#include "app/SerialUartCommandSink.hpp"

#include "hal/uart/serial.hpp"

namespace controlSystem
{
    SerialUartCommandSink::SerialUartCommandSink(transport::uart::UartTransport &rSerial)
        : mr_serial(rSerial)
    {
    }

    void SerialUartCommandSink::sendUartCommand(const char *p_logTag, uint32_t commandId)
    {
        mr_serial.sendUartCommand(p_logTag, commandId);
    }

    void SerialUartCommandSink::sendUartMessage(const char *p_logTag, UartMessage &rMessage)
    {
        mr_serial.sendUartMessage(p_logTag, rMessage);
    }
}