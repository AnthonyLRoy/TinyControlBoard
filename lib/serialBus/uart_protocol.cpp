#include "uart_protocol.hpp"

uint8_t calculate_checksum(const uint8_t *data)
{
    uint16_t sum = 0;

    // Sum bytes 1..16 exactly like Python (skip start byte, skip checksum)
    for (int i = 1; i <= 16; i++)
    {
        sum += data[i];
    }

    return sum % 256;
}

void serialize_message(UARTMessage &msg, uint8_t *buffer)
{
    buffer[0] = msg.start_byte;
    buffer[1] = msg.version;
    buffer[2] = msg.src_app;
    buffer[3] = msg.msg_type;
    buffer[4] = msg.sequence;
    buffer[5] = msg.command_id & 0xFF;
    buffer[6] = msg.command_id >> 8;

    for (int i = 0; i < 5; ++i)
    {
        buffer[7 + i * 2] = msg.params[i] & 0xFF;
        buffer[8 + i * 2] = msg.params[i] >> 8;
    }

    buffer[17] = calculate_checksum(buffer);
    msg.checksum = buffer[17];
}

bool deserialize_message(const uint8_t *buffer, UARTMessage &msg)
{
    if (buffer[0] != UART_START_BYTE)
        return false;

    msg.start_byte = buffer[0];
    msg.version = buffer[1];
    msg.src_app = buffer[2];
    msg.msg_type = buffer[3];
    msg.sequence = buffer[4];
    msg.command_id = buffer[5] | (buffer[6] << 8);

    for (int i = 0; i < 5; ++i)
    {
        msg.params[i] = buffer[7 + i * 2] | (buffer[8 + i * 2] << 8);
    }

    msg.checksum = buffer[17];
    return msg.checksum == calculate_checksum(buffer);
}