#pragma once

#include "uart_protocol.hpp"
#include <esp_log.h>
#include <cstring>

class UartReceiver {
public:
    static constexpr int FRAME_SIZE = 18;
    static constexpr int BUFFER_SIZE = 256;

    UartReceiver();

    // Feed incoming raw UART data
    void pushBytes(const uint8_t *pData, int len);

    // Try to extract a valid UartMessage
    // Returns true if a complete, valid message was parsed
    bool getNextMessage(UartMessage& rOutMsg);

private:
    uint8_t mBuffer[BUFFER_SIZE];
    int mBufferLen;
};
