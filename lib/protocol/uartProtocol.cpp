#include "uartProtocol.hpp"

uint8_t calculateChecksum(const uint8_t *p_data)
{
    uint16_t sum = 0;

    for (int i = 1; i <= 16; i++)
    {
        sum += p_data[i];
    }

    return sum % 256;
}

void serializeMessage(UartMessage &rMsg, uint8_t *p_buffer)
{
    p_buffer[0] = rMsg.startByte;
    p_buffer[1] = rMsg.version;
    p_buffer[2] = rMsg.srcApp;
    p_buffer[3] = rMsg.msgType;
    p_buffer[4] = rMsg.sequence;
    p_buffer[5] = rMsg.commandId & 0xFF;
    p_buffer[6] = rMsg.commandId >> 8;

    for (int i = 0; i < 5; ++i)
    {
        p_buffer[7 + i * 2] = rMsg.params[i] & 0xFF;
        p_buffer[8 + i * 2] = rMsg.params[i] >> 8;
    }

    p_buffer[17] = calculateChecksum(p_buffer);
    rMsg.checksum = p_buffer[17];
}

bool deserializeMessage(const uint8_t *p_buffer, UartMessage &rMsg)
{
    if (p_buffer[0] != UART_START_BYTE)
        return false;

    rMsg.startByte = p_buffer[0];
    rMsg.version = p_buffer[1];
    rMsg.srcApp = p_buffer[2];
    rMsg.msgType = p_buffer[3];
    rMsg.sequence = p_buffer[4];
    rMsg.commandId = p_buffer[5] | (p_buffer[6] << 8);

    for (int i = 0; i < 5; ++i)
    {
        rMsg.params[i] = p_buffer[7 + i * 2] | (p_buffer[8 + i * 2] << 8);
    }

    rMsg.checksum = p_buffer[17];
    return rMsg.checksum == calculateChecksum(p_buffer);
}
