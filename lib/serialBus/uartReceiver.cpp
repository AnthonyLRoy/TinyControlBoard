#include "uartReceiver.hpp"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
UartReceiver::UartReceiver() : mBufferLen(0) {}

void UartReceiver::pushBytes(const uint8_t *pData, int len) {
    int toCopy = MIN(len, BUFFER_SIZE - mBufferLen);
    memcpy(mBuffer + mBufferLen, pData, toCopy);
    mBufferLen += toCopy;
}

bool UartReceiver::getNextMessage(UartMessage& rOutMsg) {
    while (mBufferLen >= FRAME_SIZE) {
        if (mBuffer[0] == UART_START_BYTE) {
            if (deserializeMessage(mBuffer, rOutMsg)) {
                // Remove the message from the buffer
                memmove(mBuffer, mBuffer + FRAME_SIZE, mBufferLen - FRAME_SIZE);
                mBufferLen -= FRAME_SIZE;
                return true;
            } else {
                // Bad checksum, shift by 1
                memmove(mBuffer, mBuffer + 1, mBufferLen - 1);
                mBufferLen -= 1;
            }
        } else {
            // Not a valid start byte
            memmove(mBuffer, mBuffer + 1, mBufferLen - 1);
            mBufferLen -= 1;
        }
    }
    return false;
}