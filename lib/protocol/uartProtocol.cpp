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

// Serialises outgoing packets. Command packets use a fixed 10-byte params payload;
// MSG_PLAYLIST_CMD uses a variable-length playlist-name payload instead. Returns packet size.
uint8_t serializeMessage(UartMessage &rMsg, uint8_t *p_buffer)
{
    p_buffer[0] = rMsg.startByte;
    p_buffer[1] = rMsg.version;
    p_buffer[2] = rMsg.srcApp;
    p_buffer[3] = rMsg.msgType;
    p_buffer[4] = rMsg.sequence;
    p_buffer[5] = static_cast<uint8_t>(rMsg.commandId & 0xFF);
    p_buffer[6] = static_cast<uint8_t>(rMsg.commandId >> 8);

    uint8_t packetSize;
    if (rMsg.msgType == MSG_PLAYLIST_CMD)
    {
        const uint8_t len = (rMsg.playlistNameOutLen > protocol::k_maxLibraryNameLen)
                            ? protocol::k_maxLibraryNameLen : rMsg.playlistNameOutLen;
        p_buffer[7] = len;
        memcpy(p_buffer + protocol::k_headerSize, rMsg.playlistNameOut, len);
        packetSize = protocol::k_headerSize + len + 1;
    }
    else
    {
        p_buffer[7] = protocol::k_legacyPayloadSize;
        for (int i = 0; i < 5; ++i)
        {
            p_buffer[8 + i * 2] = static_cast<uint8_t>(rMsg.params[i] & 0xFF);
            p_buffer[9 + i * 2] = static_cast<uint8_t>(rMsg.params[i] >> 8);
        }
        packetSize = protocol::k_commandPacketSize;
    }

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
    else if (rMsg.msgType == MSG_TRACK_PROGRESS)
    {
        if (payloadLen != 5)
            return false;

        const uint8_t *p = p_buffer + protocol::k_headerSize;
        rMsg.trackElapsedSec  = static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
        rMsg.trackDurationSec = static_cast<uint16_t>(p[2]) | (static_cast<uint16_t>(p[3]) << 8);
        rMsg.trackIsPlaying   = p[4] != 0;
    }
    else if (rMsg.msgType == MSG_LIBRARY_ENTRY)
    {
        if (payloadLen < 5)
            return false;

        const uint8_t *p = p_buffer + protocol::k_headerSize;
        rMsg.libraryEntryType  = p[0];
        rMsg.libraryEntryIndex = static_cast<uint16_t>(p[1]) | (static_cast<uint16_t>(p[2]) << 8);
        rMsg.libraryEntryTotal = static_cast<uint16_t>(p[3]) | (static_cast<uint16_t>(p[4]) << 8);

        const uint8_t nameLen = payloadLen - 5;
        const uint8_t len = (nameLen > protocol::k_maxLibraryNameLen)
                            ? protocol::k_maxLibraryNameLen : nameLen;
        memcpy(rMsg.libraryEntryName, p + 5, len);
        rMsg.libraryEntryName[len] = '\0';
        rMsg.libraryEntryNameLen = len;
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
