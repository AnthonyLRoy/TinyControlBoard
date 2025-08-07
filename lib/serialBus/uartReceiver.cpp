#include "UartReceiver.hpp"
#define MIN(a, b) ((a) < (b) ? (a) : (b))
UartReceiver::UartReceiver() : buffer_len(0) {}

void UartReceiver::push_bytes(const uint8_t* data, int len) {
    int to_copy = MIN(len, BUFFER_SIZE - buffer_len);
    memcpy(buffer + buffer_len, data, to_copy);
    buffer_len += to_copy;
}

bool UartReceiver::get_next_message(UARTMessage& out_msg) {
    while (buffer_len >= FRAME_SIZE) {
        if (buffer[0] == UART_START_BYTE) {
            if (deserialize_message(buffer, out_msg)) {
                // Remove the message from the buffer
                memmove(buffer, buffer + FRAME_SIZE, buffer_len - FRAME_SIZE);
                buffer_len -= FRAME_SIZE;
                return true;
            } else {
                // Bad checksum, shift by 1
                memmove(buffer, buffer + 1, buffer_len - 1);
                buffer_len -= 1;
            }
        } else {
            // Not a valid start byte
            memmove(buffer, buffer + 1, buffer_len - 1);
            buffer_len -= 1;
        }
    }
    return false;
}