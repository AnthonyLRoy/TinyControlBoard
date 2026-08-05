#include "uartProtocol.hpp"

// Sums bytes [1..7+payloadLen] — covers the length byte itself in the checksum.
uint8_t calculateChecksum(const uint8_t *p_data)
{
    const uint8_t payloadLen = p_data[protocol::k_indexPayloadLen];
    uint16_t sum = 0;
    for (int i = 1; i <= protocol::k_headerSize - 1 + payloadLen; i++)
        sum += p_data[i];
    return static_cast<uint8_t>(sum % 256);
}

// Serialises outgoing command packets (params → 10-byte payload). Returns packet size.
uint8_t serializeMessage(UartMessage &rMsg, uint8_t *p_buffer)
{
    p_buffer[0] = rMsg.startByte;
    p_buffer[1] = rMsg.version;
    p_buffer[2] = rMsg.srcApp;
    p_buffer[3] = rMsg.msgType;
    p_buffer[4] = rMsg.sequence;
    p_buffer[5] = static_cast<uint8_t>(rMsg.commandId & 0xFF);
    p_buffer[6] = static_cast<uint8_t>(rMsg.commandId >> 8);
    p_buffer[7] = protocol::k_legacyPayloadSize;

    for (int i = 0; i < 5; ++i)
    {
        p_buffer[8 + i * 2] = static_cast<uint8_t>(rMsg.params[i] & 0xFF);
        p_buffer[9 + i * 2] = static_cast<uint8_t>(rMsg.params[i] >> 8);
    }

    const uint8_t packetSize = protocol::k_commandPacketSize;
    p_buffer[packetSize - 1] = calculateChecksum(p_buffer);
    rMsg.checksum = p_buffer[packetSize - 1];
    return packetSize;
}

bool deserializeMessage(const uint8_t *p_buffer, UartMessage &rMsg)
{
    if (p_buffer[0] != UART_START_BYTE)
        return false;

    rMsg.startByte = p_buffer[0];
    rMsg.version   = p_buffer[1];
    rMsg.srcApp    = p_buffer[2];
    rMsg.msgType   = p_buffer[3];
    rMsg.sequence  = p_buffer[4];
    rMsg.commandId = static_cast<uint16_t>(p_buffer[5]) | (static_cast<uint16_t>(p_buffer[6]) << 8);

    const uint8_t payloadLen = p_buffer[7];
    rMsg.checksum = p_buffer[protocol::k_headerSize + payloadLen];

    if (rMsg.checksum != calculateChecksum(p_buffer))
        return false;

    if (rMsg.msgType == MSG_NOW_PLAYING)
    {
        const uint8_t len = (payloadLen > protocol::k_maxNowPlayingLen)
                            ? protocol::k_maxNowPlayingLen : payloadLen;
        memcpy(rMsg.nowPlayingText, p_buffer + protocol::k_headerSize, len);
        rMsg.nowPlayingText[len] = '\0';
        rMsg.nowPlayingLen = len;
    }
    else
    {
        const int pairs = (payloadLen / 2 < 5) ? payloadLen / 2 : 5;
        for (int i = 0; i < pairs; ++i)
            rMsg.params[i] = static_cast<uint16_t>(p_buffer[8 + i * 2])
                           | (static_cast<uint16_t>(p_buffer[9 + i * 2]) << 8);
    }

    return true;
}
