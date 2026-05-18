#include "transport/uart/uartReceiver.hpp"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

UartReceiver::UartReceiver() : m_bufferLen(0) {}

void UartReceiver::pushBytes(const uint8_t *p_data, int len)
{
    int toCopy = MIN(len, BUFFER_SIZE - m_bufferLen);
    memcpy(m_buffer + m_bufferLen, p_data, toCopy);
    m_bufferLen += toCopy;
}

bool UartReceiver::getNextMessage(UartMessage &rOutMsg)
{
    while (m_bufferLen >= FRAME_SIZE)
    {
        if (m_buffer[0] == UART_START_BYTE)
        {
            if (deserializeMessage(m_buffer, rOutMsg))
            {
                memmove(m_buffer, m_buffer + FRAME_SIZE, m_bufferLen - FRAME_SIZE);
                m_bufferLen -= FRAME_SIZE;
                return true;
            }

            memmove(m_buffer, m_buffer + 1, m_bufferLen - 1);
            m_bufferLen -= 1;
        }
        else
        {
            memmove(m_buffer, m_buffer + 1, m_bufferLen - 1);
            m_bufferLen -= 1;
        }
    }
    return false;
}