#include "uart_protocol.hpp"

uint8_t calculate_checksum(const uint8_t *data, size_t len) {
    uint8_t chk = 0;
    for (size_t i = 0; i < len; ++i) {
        chk ^= data[i];
    }
    return chk;
}

void serialize_message( UARTMessage &msg, uint8_t *buffer) {
    buffer[0] = msg.start_byte;
    buffer[1] = msg.version;
    buffer[2] = msg.src_app;
    buffer[3] = msg.msg_type;
    buffer[4] = msg.sequence;
    buffer[5] = msg.command_id & 0xFF;
    buffer[6] = msg.command_id >> 8;

    for (int i = 0; i < 5; ++i) {
        buffer[7 + i * 2] = msg.params[i] & 0xFF;
        buffer[8 + i * 2] = msg.params[i] >> 8;
    }

    msg.checksum = calculate_checksum(&buffer[1], 16); // exclude start_byte and checksum itself
    buffer[17] = msg.checksum;
}

bool deserialize_message(uint8_t *buffer, UARTMessage &msg) {
    if (buffer[0] != UART_START_BYTE)
        return false;

    msg.start_byte = buffer[0];
    msg.version    = buffer[1];
    msg.src_app    = buffer[2];
    msg.msg_type   = buffer[3];
    msg.sequence   = buffer[4];
    msg.command_id = buffer[5] | (buffer[6] << 8);

    for (int i = 0; i < 5; ++i) {
        msg.params[i] = buffer[7 + i * 2] | (buffer[8 + i * 2] << 8);
    }

    msg.checksum = buffer[17];
    return msg.checksum == calculate_checksum(&buffer[1], 16);
}