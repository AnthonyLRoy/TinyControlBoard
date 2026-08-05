#pragma once

#include "protocol/uartProtocol.hpp"

#include <cstring>
#include <esp_log.h>

class UartReceiver
{
public:
    static constexpr int BUFFER_SIZE = 256;

    UartReceiver();

    void pushBytes(const uint8_t *p_data, int len);
    bool getNextMessage(UartMessage &rOutMsg);

private:
    uint8_t m_buffer[BUFFER_SIZE];
    int m_bufferLen;
};