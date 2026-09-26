#include "uartReceiver.hpp"

// Creates an empty receive buffer ready to accumulate UART bytes.
UartReceiver::UartReceiver() : m_bufferLen(0) {}

// Appends incoming UART bytes into the rolling receive buffer without overrunning the storage limit.
void UartReceiver::pushBytes(const uint8_t *p_data, int len)
{
    int toCopy = len;
    if (toCopy > BUFFER_SIZE - m_bufferLen)
        toCopy = BUFFER_SIZE - m_bufferLen;
    memcpy(m_buffer + m_bufferLen, p_data, toCopy);
    m_bufferLen += toCopy;
}

// Extracts the next complete message from the buffer, discarding invalid framing and resynchronizing.
bool UartReceiver::getNextMessage(UartMessage &rOutMsg)
{
    // Need the full header (8 bytes) before we know the payload length.
    while (m_bufferLen >= protocol::k_headerSize)
    {
        if (m_buffer[0] != UART_START_BYTE)
        {
            memmove(m_buffer, m_buffer + 1, m_bufferLen - 1);
            m_bufferLen -= 1;
            continue;
        }

        const int payloadLen = static_cast<int>(m_buffer[protocol::k_indexPayloadLen]);
        const int frameSize  = protocol::k_headerSize + payloadLen + 1; // +1 for checksum

        if (m_bufferLen < frameSize)
            return false; // wait for more bytes

        if (deserializeMessage(m_buffer, rOutMsg))
        {
            memmove(m_buffer, m_buffer + frameSize, m_bufferLen - frameSize);
            m_bufferLen -= frameSize;
            return true;
        }

        // Bad checksum — skip start byte and resync
        memmove(m_buffer, m_buffer + 1, m_bufferLen - 1);
        m_bufferLen -= 1;
    }
    return false;
}