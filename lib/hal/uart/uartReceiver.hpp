#pragma once

#include "protocol/uartProtocol.hpp"

#include <cstring>
#include <esp_log.h>

class UartReceiver
{
public:
    static constexpr int BUFFER_SIZE = 256;

    // Creates an empty UART receive buffer ready to accumulate incoming bytes.
    UartReceiver();

    // Appends newly received bytes to the receive buffer up to the available capacity.
    void pushBytes(const uint8_t *p_data, int len);
    // Extracts the next valid message from the buffer, resynchronizing on framing errors.
    bool getNextMessage(UartMessage &rOutMsg);

private:
    uint8_t m_buffer[BUFFER_SIZE];
    int m_bufferLen;
};