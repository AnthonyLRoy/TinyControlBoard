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

namespace
{
// Table-driven payload encode/decode: adding a new MessageType's payload layout means adding
// one function + one table row here, instead of another branch in serialize/deserializeMessage.
using Encoder = uint8_t (*)(UartMessage &rMsg, uint8_t *p_payload);
using Decoder = bool (*)(UartMessage &rMsg, const uint8_t *p_payload, uint8_t payloadLen);

uint8_t encodeLegacyParams(UartMessage &rMsg, uint8_t *p_payload)
{
    for (int i = 0; i < 5; ++i)
    {
        p_payload[i * 2]     = static_cast<uint8_t>(rMsg.params[i] & 0xFF);
        p_payload[i * 2 + 1] = static_cast<uint8_t>(rMsg.params[i] >> 8);
    }
    return protocol::k_legacyPayloadSize;
}

uint8_t encodePlaylistCmd(UartMessage &rMsg, uint8_t *p_payload)
{
    const uint8_t len = (rMsg.playlistNameOutLen > protocol::k_maxLibraryNameLen)
                        ? protocol::k_maxLibraryNameLen : rMsg.playlistNameOutLen;
    memcpy(p_payload, rMsg.playlistNameOut, len);
    return len;
}

bool decodeStandardParams(UartMessage &rMsg, const uint8_t *p_payload, uint8_t payloadLen)
{
    const int pairs = (payloadLen / 2 < 5) ? payloadLen / 2 : 5;
    for (int i = 0; i < pairs; ++i)
        rMsg.params[i] = static_cast<uint16_t>(p_payload[i * 2])
                       | (static_cast<uint16_t>(p_payload[i * 2 + 1]) << 8);
    return true;
}

bool decodeNowPlaying(UartMessage &rMsg, const uint8_t *p_payload, uint8_t payloadLen)
{
    const uint8_t len = (payloadLen > protocol::k_maxNowPlayingLen)
                        ? protocol::k_maxNowPlayingLen : payloadLen;
    memcpy(rMsg.nowPlayingText, p_payload, len);
    rMsg.nowPlayingText[len] = '\0';
    rMsg.nowPlayingLen = len;
    return true;
}

bool decodeTrackProgress(UartMessage &rMsg, const uint8_t *p_payload, uint8_t payloadLen)
{
    if (payloadLen != 5)
        return false;
    rMsg.trackElapsedSec  = static_cast<uint16_t>(p_payload[0]) | (static_cast<uint16_t>(p_payload[1]) << 8);
    rMsg.trackDurationSec = static_cast<uint16_t>(p_payload[2]) | (static_cast<uint16_t>(p_payload[3]) << 8);
    rMsg.trackIsPlaying   = p_payload[4] != 0;
    return true;
}

bool decodeLibraryEntry(UartMessage &rMsg, const uint8_t *p_payload, uint8_t payloadLen)
{
    if (payloadLen < 6)
        return false;
    rMsg.libraryEntryType  = p_payload[0];
    rMsg.libraryEntryIndex = static_cast<uint16_t>(p_payload[1]) | (static_cast<uint16_t>(p_payload[2]) << 8);
    rMsg.libraryEntryTotal = static_cast<uint16_t>(p_payload[3]) | (static_cast<uint16_t>(p_payload[4]) << 8);

    const uint8_t declaredNameLen = p_payload[5];
    const uint8_t availableNameLen = (payloadLen - 6 < declaredNameLen) ? payloadLen - 6 : declaredNameLen;
    const uint8_t nameLen = (availableNameLen > protocol::k_maxLibraryNameLen)
                           ? protocol::k_maxLibraryNameLen : availableNameLen;
    memcpy(rMsg.libraryEntryName, p_payload + 6, nameLen);
    rMsg.libraryEntryName[nameLen] = '\0';
    rMsg.libraryEntryNameLen = nameLen;

    const uint8_t albumOffset = 6 + availableNameLen;
    const uint8_t availableAlbumLen = (payloadLen > albumOffset) ? payloadLen - albumOffset : 0;
    const uint8_t albumLen = (availableAlbumLen > protocol::k_maxLibraryAlbumLen)
                            ? protocol::k_maxLibraryAlbumLen : availableAlbumLen;
    memcpy(rMsg.libraryEntryAlbum, p_payload + albumOffset, albumLen);
    rMsg.libraryEntryAlbum[albumLen] = '\0';
    rMsg.libraryEntryAlbumLen = albumLen;
    return true;
}

bool decodePlaylistResult(UartMessage &rMsg, const uint8_t *p_payload, uint8_t payloadLen)
{
    if (payloadLen < 1)
        return false;
    rMsg.playlistResultOk = p_payload[0] != 0;
    const uint8_t msgLen = payloadLen - 1;
    const uint8_t len = (msgLen > protocol::k_maxLibraryNameLen)
                       ? protocol::k_maxLibraryNameLen : msgLen;
    memcpy(rMsg.playlistResultMessage, p_payload + 1, len);
    rMsg.playlistResultMessage[len] = '\0';
    rMsg.playlistResultMessageLen = len;
    return true;
}

struct EncoderEntry { uint8_t msgType; Encoder encode; };
const EncoderEntry k_encoders[] = {
    { MSG_PLAYLIST_CMD, encodePlaylistCmd },
};

struct DecoderEntry { uint8_t msgType; Decoder decode; };
const DecoderEntry k_decoders[] = {
    { MSG_NOW_PLAYING,     decodeNowPlaying },
    { MSG_TRACK_PROGRESS,  decodeTrackProgress },
    { MSG_LIBRARY_ENTRY,   decodeLibraryEntry },
    { MSG_PLAYLIST_RESULT, decodePlaylistResult },
};
} // namespace

// Serialises outgoing packets. Most message types use a fixed 10-byte params payload
// (encodeLegacyParams); table-driven per-type encoders (see k_encoders) override that for
// message types with a different payload layout, e.g. MSG_PLAYLIST_CMD. Returns packet size.
uint8_t serializeMessage(UartMessage &rMsg, uint8_t *p_buffer)
{
    p_buffer[0] = rMsg.startByte;
    p_buffer[1] = rMsg.version;
    p_buffer[2] = rMsg.srcApp;
    p_buffer[3] = rMsg.msgType;
    p_buffer[4] = rMsg.sequence;
    p_buffer[5] = static_cast<uint8_t>(rMsg.commandId & 0xFF);
    p_buffer[6] = static_cast<uint8_t>(rMsg.commandId >> 8);

    uint8_t *p_payload = p_buffer + protocol::k_headerSize;
    uint8_t payloadLen = encodeLegacyParams(rMsg, p_payload);
    for (const auto &entry : k_encoders)
    {
        if (entry.msgType == rMsg.msgType)
        {
            payloadLen = entry.encode(rMsg, p_payload);
            break;
        }
    }
    p_buffer[7] = payloadLen;

    const uint8_t packetSize = static_cast<uint8_t>(protocol::k_headerSize + payloadLen + 1);
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

    const uint8_t *p_payload = p_buffer + protocol::k_headerSize;
    for (const auto &entry : k_decoders)
    {
        if (entry.msgType == rMsg.msgType)
            return entry.decode(rMsg, p_payload, payloadLen);
    }
    return decodeStandardParams(rMsg, p_payload, payloadLen);
}

