#include "uart_protocol.hpp"

uint8_t calculateChecksum(const uint8_t *pData)
{
    uint16_t sum = 0;

    // Sum bytes 1..16 exactly like Python otherwise we are fucked (skip start byte, skip checksum)
    for (int i = 1; i <= 16; i++)
    {
        sum += pData[i];
    }

    return sum % 256;
}

void serializeMessage(UartMessage &rMsg, uint8_t *pBuffer)
{
    pBuffer[0] = rMsg.startByte;
    pBuffer[1] = rMsg.version;
    pBuffer[2] = rMsg.srcApp;
    pBuffer[3] = rMsg.msgType;
    pBuffer[4] = rMsg.sequence;
    pBuffer[5] = rMsg.commandId & 0xFF;
    pBuffer[6] = rMsg.commandId >> 8;

    for (int i = 0; i < 5; ++i)
    {
        pBuffer[7 + i * 2] = rMsg.params[i] & 0xFF;
        pBuffer[8 + i * 2] = rMsg.params[i] >> 8;
    }

    pBuffer[17] = calculateChecksum(pBuffer);
    rMsg.checksum = pBuffer[17];
}

bool deserializeMessage(const uint8_t *pBuffer, UartMessage &rMsg)
{
    if (pBuffer[0] != UART_START_BYTE)
        return false;

    rMsg.startByte = pBuffer[0];
    rMsg.version = pBuffer[1];
    rMsg.srcApp = pBuffer[2];
    rMsg.msgType = pBuffer[3];
    rMsg.sequence = pBuffer[4];
    rMsg.commandId = pBuffer[5] | (pBuffer[6] << 8);

    for (int i = 0; i < 5; ++i)
    {
        rMsg.params[i] = pBuffer[7 + i * 2] | (pBuffer[8 + i * 2] << 8);
    }

    rMsg.checksum = pBuffer[17];
    return rMsg.checksum == calculateChecksum(pBuffer);
}