#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "actions/SimpleCommandAction.hpp"
#include "protocol/uartProtocol.hpp"

namespace
{
template <typename TExpected, typename TActual>
void expect_equal(const TExpected &expected, const TActual &actual, const std::string &message)
{
    if (expected != actual)
    {
        std::ostringstream stream;
        stream << message << " expected=" << expected << " actual=" << actual;
        throw std::runtime_error(stream.str());
    }
}

void expect_true(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void test_simple_command_action_returns_configured_command_when_pressed()
{
    actions::SimpleCommandAction action(CMD_PLAY_PAUSE);
    const actions::ActionResponse response = action.execute(true);

    expect_equal(CMD_PLAY_PAUSE, response.command, "Pressed action should return configured command");
    expect_true(!response.isActive, "Pressed action should keep default inactive state");
    expect_true(!response.keepLedActive, "Pressed action should not force LED activity");
    expect_equal(static_cast<uint16_t>(0), response.releaseTimeMillis, "Pressed action should keep default release time");
}

void test_simple_command_action_returns_no_action_when_released()
{
    actions::SimpleCommandAction action(CMD_PLAY_PAUSE);
    const actions::ActionResponse response = action.execute(false);

    expect_equal(CMD_NO_ACTION, response.command, "Released action should not emit a command");
    expect_true(!response.isActive, "Released action should keep default inactive state");
    expect_true(!response.keepLedActive, "Released action should not force LED activity");
    expect_equal(static_cast<uint16_t>(0), response.releaseTimeMillis, "Released action should keep default release time");
}

void test_uart_message_defaults_match_protocol()
{
    UartMessage message;

    expect_equal(static_cast<uint8_t>(UART_START_BYTE), message.startByte, "Default start byte should match protocol");
    expect_equal(static_cast<uint8_t>(UART_PROTOCOL_VERSION), message.version, "Default protocol version should match protocol");
    expect_equal(static_cast<uint8_t>(APP_ESP32), message.srcApp, "Default source app should be ESP32");
    expect_equal(static_cast<uint8_t>(MSG_COMMAND), message.msgType, "Default message type should be command");
    expect_equal(static_cast<uint8_t>(0), message.sequence, "Default sequence should be zero");
    expect_equal(static_cast<uint16_t>(CMD_NO_ACTION), message.commandId, "Default command should be no action");
    expect_equal(static_cast<uint8_t>(0), message.checksum, "Default checksum should be zero");

    for (uint16_t parameter : message.params)
    {
        expect_equal(static_cast<uint16_t>(0), parameter, "Default parameters should be cleared");
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

    expect_equal(static_cast<uint8_t>(UART_START_BYTE), buffer[0], "Serialized start byte mismatch");
    expect_equal(static_cast<uint8_t>(UART_PROTOCOL_VERSION), buffer[1], "Serialized version mismatch");
    expect_equal(static_cast<uint8_t>(APP_PI), buffer[2], "Serialized source app mismatch");
    expect_equal(static_cast<uint8_t>(MSG_STATUS), buffer[3], "Serialized message type mismatch");
    expect_equal(static_cast<uint8_t>(0x23), buffer[4], "Serialized sequence mismatch");
    expect_equal(static_cast<uint8_t>(0x03), buffer[5], "Serialized command low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x00), buffer[6], "Serialized command high byte mismatch");
    expect_equal(static_cast<uint8_t>(0x34), buffer[7], "Serialized parameter 0 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x12), buffer[8], "Serialized parameter 0 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0x78), buffer[9], "Serialized parameter 1 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x56), buffer[10], "Serialized parameter 1 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0xBC), buffer[11], "Serialized parameter 2 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x9A), buffer[12], "Serialized parameter 2 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0xF0), buffer[13], "Serialized parameter 3 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0xDE), buffer[14], "Serialized parameter 3 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0x57), buffer[15], "Serialized parameter 4 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x13), buffer[16], "Serialized parameter 4 high byte mismatch");
    expect_equal(calculateChecksum(buffer), buffer[17], "Serialized checksum mismatch");
    expect_equal(buffer[17], message.checksum, "Serialized checksum should be written back into the message");
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

    expect_true(deserializeMessage(buffer, parsed), "Round-trip deserialize should succeed");
    expect_equal(original.startByte, parsed.startByte, "Round-trip start byte mismatch");
    expect_equal(original.version, parsed.version, "Round-trip version mismatch");
    expect_equal(original.srcApp, parsed.srcApp, "Round-trip source app mismatch");
    expect_equal(original.msgType, parsed.msgType, "Round-trip message type mismatch");
    expect_equal(original.sequence, parsed.sequence, "Round-trip sequence mismatch");
    expect_equal(original.commandId, parsed.commandId, "Round-trip command mismatch");
    expect_equal(original.checksum, parsed.checksum, "Round-trip checksum mismatch");

    for (int index = 0; index < 5; ++index)
    {
        expect_equal(original.params[index], parsed.params[index], "Round-trip parameter mismatch");
    }
}

void test_deserialize_message_rejects_invalid_start_byte()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};

    buffer[0] = 0x55;

    expect_true(!deserializeMessage(buffer, parsed), "Invalid start byte should be rejected");
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

    expect_true(!deserializeMessage(buffer, parsed), "Invalid checksum should be rejected");
}
} // namespace

int main()
{
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"test_simple_command_action_returns_configured_command_when_pressed", test_simple_command_action_returns_configured_command_when_pressed},
        {"test_simple_command_action_returns_no_action_when_released", test_simple_command_action_returns_no_action_when_released},
        {"test_uart_message_defaults_match_protocol", test_uart_message_defaults_match_protocol},
        {"test_serialize_message_writes_expected_fields_and_checksum", test_serialize_message_writes_expected_fields_and_checksum},
        {"test_deserialize_message_round_trips_serialized_message", test_deserialize_message_round_trips_serialized_message},
        {"test_deserialize_message_rejects_invalid_start_byte", test_deserialize_message_rejects_invalid_start_byte},
        {"test_deserialize_message_rejects_invalid_checksum", test_deserialize_message_rejects_invalid_checksum},
    };

    int failures = 0;
    for (const auto &test : tests)
    {
        try
        {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        }
        catch (const std::exception &exception)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << '\n';
        }
    }

    std::cout << tests.size() - failures << "/" << tests.size() << " tests passed" << std::endl;
    return failures == 0 ? 0 : 1;
}