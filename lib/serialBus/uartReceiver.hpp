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
    void push_bytes(const uint8_t* data, int len);

    // Try to extract a valid UARTMessage
    // Returns true if a complete, valid message was parsed
    bool get_next_message(UARTMessage& out_msg);

private:
    uint8_t buffer[BUFFER_SIZE];
    int buffer_len;
};
