#pragma once

#include "protocol/uartProtocol.hpp"

#include <cstring>
#include <esp_log.h>

class UartReceiver
{
public:
    static constexpr int FRAME_SIZE = 18;
    static constexpr int BUFFER_SIZE = 256;

    UartReceiver();

    void pushBytes(const uint8_t *pData, int len);
    bool getNextMessage(UartMessage &rOutMsg);

private:
    uint8_t mBuffer[BUFFER_SIZE];
    int mBufferLen;
};