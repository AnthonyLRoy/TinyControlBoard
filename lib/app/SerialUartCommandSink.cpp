#include "app/SerialUartCommandSink.hpp"

#include "transport/uart/serial.hpp"

namespace controlSystem
{
    SerialUartCommandSink::SerialUartCommandSink(transport::uart::Serial &rSerial)
        : mrSerial(rSerial)
    {
    }

    void SerialUartCommandSink::sendUartCommand(const char *pLogTag, uint32_t commandId)
    {
        mrSerial.sendUartCommand(pLogTag, commandId);
    }

    void SerialUartCommandSink::sendUartMessage(const char *pLogTag, UartMessage &rMessage)
    {
        mrSerial.sendUartMessage(pLogTag, rMessage);
    }
}