#include "SerialUartCommandSink.hpp"

#include "serial.hpp"

namespace controlSystem
{
    SerialUartCommandSink::SerialUartCommandSink(serialBus::Serial &rSerial)
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
