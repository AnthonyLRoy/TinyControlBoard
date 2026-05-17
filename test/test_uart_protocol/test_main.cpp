#include <unity.h>

#include "uartProtocol.hpp"

void test_uart_message_defaults_match_protocol()
{
    UartMessage message;

    TEST_ASSERT_EQUAL_HEX8(UART_START_BYTE, message.startByte);
    TEST_ASSERT_EQUAL_HEX8(UART_PROTOCOL_VERSION, message.version);
    TEST_ASSERT_EQUAL_HEX8(APP_ESP32, message.srcApp);
    TEST_ASSERT_EQUAL_HEX8(MSG_COMMAND, message.msgType);
    TEST_ASSERT_EQUAL_HEX8(0, message.sequence);
    TEST_ASSERT_EQUAL_HEX16(CMD_NO_ACTION, message.commandId);
    TEST_ASSERT_EQUAL_HEX8(0, message.checksum);

    for (int index = 0; index < 5; ++index)
    {
        TEST_ASSERT_EQUAL_HEX16(0, message.params[index]);
    }
}

void test_serialize_message_writes_expected_fields_and_checksum()
{
    UartMessage message;
    uint8_t buffer[UART_PACKET_SIZE] = {};

    message.srcApp = APP_PI;
    message.msgType = MSG_STATUS;
    message.sequence = 0x23;
    message.commandId = CMD_SYS_HEARTBEAT;
    message.params[0] = 0x1234;
    message.params[1] = 0x5678;
    message.params[2] = 0x9ABC;
    message.params[3] = 0xDEF0;
    message.params[4] = 0x1357;

    serializeMessage(message, buffer);

    TEST_ASSERT_EQUAL_HEX8(UART_START_BYTE, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(UART_PROTOCOL_VERSION, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(APP_PI, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(MSG_STATUS, buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0x23, buffer[4]);
    TEST_ASSERT_EQUAL_HEX8(0x03, buffer[5]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[6]);
    TEST_ASSERT_EQUAL_HEX8(0x34, buffer[7]);
    TEST_ASSERT_EQUAL_HEX8(0x12, buffer[8]);
    TEST_ASSERT_EQUAL_HEX8(0x78, buffer[9]);
    TEST_ASSERT_EQUAL_HEX8(0x56, buffer[10]);
    TEST_ASSERT_EQUAL_HEX8(0xBC, buffer[11]);
    TEST_ASSERT_EQUAL_HEX8(0x9A, buffer[12]);
    TEST_ASSERT_EQUAL_HEX8(0xF0, buffer[13]);
    TEST_ASSERT_EQUAL_HEX8(0xDE, buffer[14]);
    TEST_ASSERT_EQUAL_HEX8(0x57, buffer[15]);
    TEST_ASSERT_EQUAL_HEX8(0x13, buffer[16]);
    TEST_ASSERT_EQUAL_HEX8(calculateChecksum(buffer), buffer[17]);
    TEST_ASSERT_EQUAL_HEX8(buffer[17], message.checksum);
}

void test_deserialize_message_round_trips_serialized_message()
{
    UartMessage original;
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};

    original.srcApp = APP_PI;
    original.msgType = MSG_ACK;
    original.sequence = 0x7E;
    original.commandId = CMD_ROTARY_RIGHT;
    original.params[0] = 1;
    original.params[1] = 2;
    original.params[2] = 3;
    original.params[3] = 4;
    original.params[4] = 5;

    serializeMessage(original, buffer);

    TEST_ASSERT_TRUE(deserializeMessage(buffer, parsed));
    TEST_ASSERT_EQUAL_HEX8(original.startByte, parsed.startByte);
    TEST_ASSERT_EQUAL_HEX8(original.version, parsed.version);
    TEST_ASSERT_EQUAL_HEX8(original.srcApp, parsed.srcApp);
    TEST_ASSERT_EQUAL_HEX8(original.msgType, parsed.msgType);
    TEST_ASSERT_EQUAL_HEX8(original.sequence, parsed.sequence);
    TEST_ASSERT_EQUAL_HEX16(original.commandId, parsed.commandId);
    TEST_ASSERT_EQUAL_HEX8(original.checksum, parsed.checksum);

    for (int index = 0; index < 5; ++index)
    {
        TEST_ASSERT_EQUAL_HEX16(original.params[index], parsed.params[index]);
    }
}

void test_deserialize_message_rejects_invalid_start_byte()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};

    buffer[0] = 0x55;

    TEST_ASSERT_FALSE(deserializeMessage(buffer, parsed));
}

void test_deserialize_message_rejects_invalid_checksum()
{
    UartMessage parsed;
    UartMessage original;
    uint8_t buffer[UART_PACKET_SIZE] = {};

    original.commandId = CMD_PLAY_PAUSE;
    original.sequence = 9;

    serializeMessage(original, buffer);
    buffer[17] ^= 0xFF;

    TEST_ASSERT_FALSE(deserializeMessage(buffer, parsed));
}

extern "C" void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uart_message_defaults_match_protocol);
    RUN_TEST(test_serialize_message_writes_expected_fields_and_checksum);
    RUN_TEST(test_deserialize_message_round_trips_serialized_message);
    RUN_TEST(test_deserialize_message_rejects_invalid_start_byte);
    RUN_TEST(test_deserialize_message_rejects_invalid_checksum);
    UNITY_END();
}