#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hal/uart/heartbeatWatchdog.hpp"
#include "indicators/activityStatus.hpp"
#include "input/actions/actionTemplates.hpp"
#include "app/ActionCommandRoutingPolicy.hpp"
#include "app/ActionFactory.hpp"
#include "app/ActionUartDispatcher.hpp"
#include "app/ControlBoardActionRegistry.hpp"
#include "app/ControlBoardButtonIds.hpp"
#include "app/ControlBoardInputDispatcher.hpp"
#include "app/commands/BrightnessAction.hpp"
#include "app/commands/PowerTransitionAction.hpp"
#include "app/commands/RelayAction.hpp"
#include "app/commands/SystemAction.hpp"
#include "app/commands/UartDispatchAction.hpp"
#include "power/PowerStateTransitionPolicy.hpp"
#include "app/SerialHeartbeatRouter.hpp"
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
    const auto result = action.produce(true);

    expect_true(result != nullptr, "Press should produce an action");
    expect_equal(CMD_PLAY_PAUSE, result->command, "Pressed action should return configured command");
    expect_true(!result->isActive, "Pressed action should keep default inactive state");
    expect_equal(static_cast<uint16_t>(0), result->releaseTimeMillis, "Pressed action should keep default release time");
}

void test_simple_command_action_returns_no_action_when_released()
{
    actions::SimpleCommandAction action(CMD_PLAY_PAUSE);
    const auto result = action.produce(false);

    expect_true(result == nullptr, "Released simple command action should produce no action");
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
    expect_equal(static_cast<uint8_t>(protocol::k_legacyPayloadSize), buffer[7], "Serialized payload length mismatch");
    expect_equal(static_cast<uint8_t>(0x34), buffer[8], "Serialized parameter 0 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x12), buffer[9], "Serialized parameter 0 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0x78), buffer[10], "Serialized parameter 1 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x56), buffer[11], "Serialized parameter 1 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0xBC), buffer[12], "Serialized parameter 2 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x9A), buffer[13], "Serialized parameter 2 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0xF0), buffer[14], "Serialized parameter 3 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0xDE), buffer[15], "Serialized parameter 3 high byte mismatch");
    expect_equal(static_cast<uint8_t>(0x57), buffer[16], "Serialized parameter 4 low byte mismatch");
    expect_equal(static_cast<uint8_t>(0x13), buffer[17], "Serialized parameter 4 high byte mismatch");
    expect_equal(calculateChecksum(buffer), buffer[18], "Serialized checksum mismatch");
    expect_equal(buffer[18], message.checksum, "Serialized checksum should be written back into the message");
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

// Hand-builds a header+payload+checksum frame for message types that the firmware only ever
// receives (never encodes itself), so deserializeMessage can be exercised without a C++ encoder.
uint8_t build_frame(uint8_t msgType, uint16_t commandId, const uint8_t *p_payload, uint8_t payloadLen, uint8_t *p_buffer)
{
    p_buffer[0] = UART_START_BYTE;
    p_buffer[1] = UART_PROTOCOL_VERSION;
    p_buffer[2] = APP_PI;
    p_buffer[3] = msgType;
    p_buffer[4] = 0;
    p_buffer[5] = static_cast<uint8_t>(commandId & 0xFF);
    p_buffer[6] = static_cast<uint8_t>(commandId >> 8);
    p_buffer[7] = payloadLen;
    memcpy(p_buffer + protocol::k_headerSize, p_payload, payloadLen);
    const uint8_t packetSize = static_cast<uint8_t>(protocol::k_headerSize + payloadLen + 1);
    p_buffer[packetSize - 1] = calculateChecksum(p_buffer);
    return packetSize;
}

void test_serialize_message_playlist_cmd_writes_variable_length_payload()
{
    UartMessage message;
    uint8_t buffer[UART_PACKET_SIZE] = {};

    message.msgType = MSG_PLAYLIST_CMD;
    message.commandId = CMD_PLAYLIST_SAVE;
    const char *name = "MyPlaylist";
    message.playlistNameOutLen = static_cast<uint8_t>(strlen(name));
    memcpy(message.playlistNameOut, name, message.playlistNameOutLen);

    const uint8_t packetSize = serializeMessage(message, buffer);

    expect_equal(static_cast<uint8_t>(protocol::k_headerSize + 10 + 1), packetSize, "Playlist-cmd packet size mismatch");
    expect_equal(static_cast<uint8_t>(10), buffer[7], "Playlist-cmd payload length mismatch");
    expect_equal(std::string("MyPlaylist"),
                 std::string(reinterpret_cast<const char *>(buffer + protocol::k_headerSize), 10),
                 "Playlist-cmd name payload mismatch");
    expect_equal(calculateChecksum(buffer), buffer[packetSize - 1], "Playlist-cmd checksum mismatch");
}

void test_deserialize_message_now_playing_payload()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const uint8_t payload[] = {'H', 'e', 'l', 'l', 'o'};

    build_frame(MSG_NOW_PLAYING, 0, payload, sizeof(payload), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Now-playing deserialize should succeed");
    expect_equal(static_cast<uint8_t>(5), parsed.nowPlayingLen, "Now-playing length mismatch");
    expect_equal(std::string("Hello"), std::string(reinterpret_cast<const char *>(parsed.nowPlayingText)), "Now-playing text mismatch");
}

void test_deserialize_message_track_progress_payload()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const uint8_t payload[5] = {0x2C, 0x01, 0x58, 0x02, 0x01}; // elapsed=300 duration=600 playing=true

    build_frame(MSG_TRACK_PROGRESS, 0, payload, sizeof(payload), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Track-progress deserialize should succeed");
    expect_equal(static_cast<uint16_t>(300), parsed.trackElapsedSec, "Elapsed seconds mismatch");
    expect_equal(static_cast<uint16_t>(600), parsed.trackDurationSec, "Duration seconds mismatch");
    expect_true(parsed.trackIsPlaying, "isPlaying should be true");
}

void test_deserialize_message_library_entry_payload()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    // track, index=2, total=5, nameLen=5, name="Track", albumLen=5, album="Album", hashLen=0
    const uint8_t payload[] = {1, 0x02, 0x00, 0x05, 0x00, 5, 'T', 'r', 'a', 'c', 'k', 5, 'A', 'l', 'b', 'u', 'm', 0};

    build_frame(MSG_LIBRARY_ENTRY, 0, payload, sizeof(payload), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Library-entry deserialize should succeed");
    expect_equal(static_cast<uint8_t>(1), parsed.libraryEntryType, "Entry type mismatch");
    expect_equal(static_cast<uint16_t>(2), parsed.libraryEntryIndex, "Entry index mismatch");
    expect_equal(static_cast<uint16_t>(5), parsed.libraryEntryTotal, "Entry total mismatch");
    expect_equal(std::string("Track"), std::string(reinterpret_cast<const char *>(parsed.libraryEntryName)), "Entry name mismatch");
    expect_equal(std::string("Album"), std::string(reinterpret_cast<const char *>(parsed.libraryEntryAlbum)), "Entry album mismatch");
    expect_equal(static_cast<uint8_t>(0), parsed.libraryEntryArtHashLen, "Entry hash should be empty when absent");
}

void test_deserialize_message_library_entry_payload_with_art_hash()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const std::string hash = "0123456789abcdef0123456789abcdef"; // 32 chars
    std::vector<uint8_t> payload = {1, 0x00, 0x00, 0x01, 0x00, 3, 'F', 'o', 'o', 2, 'A', 'B',
                                     static_cast<uint8_t>(hash.size())};
    payload.insert(payload.end(), hash.begin(), hash.end());

    build_frame(MSG_LIBRARY_ENTRY, 0, payload.data(), static_cast<uint8_t>(payload.size()), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Library-entry-with-hash deserialize should succeed");
    expect_equal(std::string("Foo"), std::string(reinterpret_cast<const char *>(parsed.libraryEntryName)), "Entry name mismatch");
    expect_equal(std::string("AB"), std::string(reinterpret_cast<const char *>(parsed.libraryEntryAlbum)), "Entry album mismatch");
    expect_equal(static_cast<uint8_t>(32), parsed.libraryEntryArtHashLen, "Entry hash length mismatch");
    expect_equal(hash, std::string(reinterpret_cast<const char *>(parsed.libraryEntryArtHash)), "Entry hash mismatch");
}

// Worst-case boundary: max-length name (55) + max-length album (40) + full hash (32) all present
// at once — proves decodeLibraryEntry doesn't overrun its fixed buffers, and that the resulting
// BLE notify payload (built the same way by BleServer.cpp::notifyLibraryEntry) stays within
// protocol::k_libraryEntryMaxPayloadSize (which is itself statically asserted to fit the BLE
// ATT MTU headroom — see uartProtocol.hpp).
void test_deserialize_message_library_entry_payload_max_lengths()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const std::string name(protocol::k_maxLibraryNameLen, 'N');
    const std::string album(protocol::k_maxLibraryAlbumLen, 'A');
    const std::string hash(protocol::k_maxLibraryArtHashLen, 'H');

    std::vector<uint8_t> payload = {1, 0xFF, 0x00, 0xFF, 0x00, static_cast<uint8_t>(name.size())};
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back(static_cast<uint8_t>(album.size()));
    payload.insert(payload.end(), album.begin(), album.end());
    payload.push_back(static_cast<uint8_t>(hash.size()));
    payload.insert(payload.end(), hash.begin(), hash.end());

    expect_true(payload.size() <= protocol::k_libraryEntryMaxPayloadSize,
                "Test payload should fit within the declared max payload size");

    build_frame(MSG_LIBRARY_ENTRY, 0, payload.data(), static_cast<uint8_t>(payload.size()), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Max-length library-entry deserialize should succeed");
    expect_equal(name, std::string(reinterpret_cast<const char *>(parsed.libraryEntryName)), "Max-length name mismatch");
    expect_equal(album, std::string(reinterpret_cast<const char *>(parsed.libraryEntryAlbum)), "Max-length album mismatch");
    expect_equal(hash, std::string(reinterpret_cast<const char *>(parsed.libraryEntryArtHash)), "Max-length hash mismatch");
}

void test_deserialize_message_playlist_result_payload()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const uint8_t payload[] = {1, 'O', 'K'}; // ok=true, message="OK"

    build_frame(MSG_PLAYLIST_RESULT, 0, payload, sizeof(payload), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Playlist-result deserialize should succeed");
    expect_true(parsed.playlistResultOk, "playlistResultOk should be true");
    expect_equal(static_cast<uint8_t>(2), parsed.playlistResultMessageLen, "Playlist-result message length mismatch");
    expect_equal(std::string("OK"), std::string(reinterpret_cast<const char *>(parsed.playlistResultMessage)), "Playlist-result message mismatch");
}

void test_deserialize_message_playlist_result_failure_payload()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const uint8_t payload[] = {0, 'E', 'r', 'r'}; // ok=false, message="Err"

    build_frame(MSG_PLAYLIST_RESULT, 0, payload, sizeof(payload), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Playlist-result failure deserialize should succeed");
    expect_true(!parsed.playlistResultOk, "playlistResultOk should be false");
    expect_equal(std::string("Err"), std::string(reinterpret_cast<const char *>(parsed.playlistResultMessage)), "Playlist-result failure message mismatch");
}

void test_serialize_message_clamps_playlist_name_to_protocol_limit()
{
    UartMessage message;
    uint8_t buffer[UART_PACKET_SIZE] = {};

    message.msgType = MSG_PLAYLIST_CMD;
    message.playlistNameOutLen = 255;
    for (uint8_t index = 0; index < protocol::k_maxLibraryNameLen; ++index)
    {
        message.playlistNameOut[index] = static_cast<uint8_t>('A' + (index % 26));
    }

    const uint8_t packetSize = serializeMessage(message, buffer);

    expect_equal(static_cast<uint8_t>(protocol::k_headerSize + protocol::k_maxLibraryNameLen + 1),
                 packetSize, "Playlist name should be clamped to the maximum payload length");
    expect_equal(protocol::k_maxLibraryNameLen, buffer[protocol::k_indexPayloadLen],
                 "Clamped playlist payload length mismatch");
    expect_equal(calculateChecksum(buffer), buffer[packetSize - 1],
                 "Clamped playlist checksum mismatch");
}

void test_deserialize_message_rejects_invalid_fixed_payload_lengths()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const uint8_t shortTrackProgress[] = {0, 0, 0, 0};
    const uint8_t shortLibraryEntry[] = {0, 0, 0, 0};
    const uint8_t emptyPlaylistResult[1] = {0};

    const uint8_t trackSize = build_frame(MSG_TRACK_PROGRESS, 0, shortTrackProgress,
                                           sizeof(shortTrackProgress), buffer);
    expect_true(!deserializeMessage(buffer, parsed), "Short track-progress payload should be rejected");

    const uint8_t librarySize = build_frame(MSG_LIBRARY_ENTRY, 0, shortLibraryEntry,
                                             sizeof(shortLibraryEntry), buffer);
    expect_true(!deserializeMessage(buffer, parsed), "Short library-entry payload should be rejected");

    const uint8_t resultSize = build_frame(MSG_PLAYLIST_RESULT, 0, emptyPlaylistResult, 0, buffer);
    expect_equal(static_cast<uint8_t>(protocol::k_headerSize + 1), resultSize,
                 "Empty playlist-result frame size mismatch");
    expect_true(!deserializeMessage(buffer, parsed), "Empty playlist-result payload should be rejected");
}

void test_deserialize_message_ignores_incomplete_standard_parameter_pair()
{
    UartMessage parsed;
    uint8_t buffer[UART_PACKET_SIZE] = {};
    const uint8_t payload[] = {0x34, 0x12, 0x78};

    build_frame(MSG_COMMAND, CMD_PLAY_PAUSE, payload, sizeof(payload), buffer);

    expect_true(deserializeMessage(buffer, parsed), "Odd-length standard payload should be accepted");
    expect_equal(static_cast<uint16_t>(0x1234), parsed.params[0],
                 "First complete standard parameter should be decoded");
    expect_equal(static_cast<uint16_t>(0), parsed.params[1],
                 "Incomplete standard parameter pair should be ignored");
}

class FakeAction : public actions::IActionSource
{
public:
    explicit FakeAction(CommandId command) : m_command(command) {}

    std::unique_ptr<actions::IAction> produce(bool isPressed) override
    {
        lastPressedArg = isPressed;
        ++callCount;
        if (m_command == CMD_NO_ACTION)
            return nullptr;
        return controlSystem::createAction(m_command);
    }

    void syncExternalState(bool enabled) override
    {
        ++syncCallCount;
        lastSyncState = enabled;
    }

    int callCount = 0;
    bool lastPressedArg = false;
    int syncCallCount = 0;
    bool lastSyncState = false;

private:
    CommandId m_command;
};

class FakeResponseSink
{
public:
    void process(std::unique_ptr<actions::IAction> iaction)
    {
        ++callCount;
        lastAction = std::move(iaction);
    }

    int callCount = 0;
    std::unique_ptr<actions::IAction> lastAction;
};

class FakeIndicators : public controlSystem::IControlBoardIndicators
{
public:
    void setActivityStatus(ControlBoardWorkingStatus status) override
    {
        activityHistory.push_back(status);
    }

    void setButtonLed(uint8_t pin, bool enabled) override
    {
        ++ledCallCount;
        lastLedPin = pin;
        lastLedState = enabled;
    }

    std::vector<ControlBoardWorkingStatus> activityHistory;
    int ledCallCount = 0;
    uint8_t lastLedPin = 0;
    bool lastLedState = false;
};

class FakeUartCommandSink : public controlSystem::IUartCommandSink
{
public:
    void sendUartCommand(const char *p_logTag, uint32_t commandId) override
    {
        ++commandCount;
        lastLogTag = p_logTag;
        lastCommandId = commandId;
    }

    void sendUartMessage(const char *p_logTag, UartMessage &rMessage) override
    {
        ++messageCount;
        lastLogTag = p_logTag;
        lastMessage = rMessage;
    }

    int commandCount = 0;
    int messageCount = 0;
    std::string lastLogTag;
    uint32_t lastCommandId = 0;
    UartMessage lastMessage;
};

std::unique_ptr<actions::IAction> makeAction(CommandId command)
{
    return controlSystem::createAction(command);
}

void test_control_board_button_press_dispatches_action_and_led()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction action(CMD_PLAY_PAUSE);
    actionMap[controlSystem::controlBoardButtons::k_playPause] = {&action, controlSystem::LedPolicy::Momentary};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_playPause);

    expect_equal(1, action.callCount, "Press should call produce() once");
    expect_true(action.lastPressedArg, "Press should pass true to produce()");
    expect_equal(1, responseSink.callCount, "Press should forward the action");
    expect_equal(CMD_PLAY_PAUSE, responseSink.lastAction->command, "Press should forward returned command");
    expect_equal(1, indicators.ledCallCount, "Press should update the nonzero button LED");
    expect_equal(controlSystem::controlBoardButtons::k_playPause, indicators.lastLedPin, "Press should target the correct button LED");
    expect_true(indicators.lastLedState, "Press should turn the button LED on");
    expect_equal(static_cast<size_t>(1), indicators.activityHistory.size(), "Press should record one status update");
    expect_true(indicators.activityHistory[0] == ControlBoardWorkingStatus::doingWork,
                "Press should set doingWork status");
}

void test_control_board_momentary_button_release_turns_led_off()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction action(CMD_NO_ACTION);
    actionMap[controlSystem::controlBoardButtons::k_playPause] = {&action, controlSystem::LedPolicy::Momentary};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);
    dispatcher.handleButtonReleased(controlSystem::controlBoardButtons::k_playPause);

    expect_equal(1, action.callCount, "Release should execute mapped action exactly once");
    expect_true(!action.lastPressedArg, "Release should execute action with false");
    expect_equal(0, responseSink.callCount, "Release of simple command should not forward an action (no-op on release)");
    expect_equal(1, indicators.ledCallCount, "Momentary release should turn LED off");
    expect_equal(controlSystem::controlBoardButtons::k_playPause, indicators.lastLedPin, "Release should target the correct button LED");
    expect_true(!indicators.lastLedState, "Momentary release should set LED to false");
    expect_equal(static_cast<size_t>(1), indicators.activityHistory.size(), "Release should record one status update");
    expect_true(indicators.activityHistory[0] == ControlBoardWorkingStatus::Idle,
                "Release should set Idle status");
}

void test_control_board_toggle_button_press_flips_led_state()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction action(CMD_NO_ACTION);
    actionMap[controlSystem::controlBoardButtons::k_cover] = {&action, controlSystem::LedPolicy::Toggle};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_cover);
    expect_equal(1, indicators.ledCallCount, "First press should call setButtonLed once");
    expect_true(indicators.lastLedState, "First press should turn toggle LED on");

    dispatcher.handleButtonReleased(controlSystem::controlBoardButtons::k_cover);
    expect_equal(1, indicators.ledCallCount, "Toggle release should not call setButtonLed again");

    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_cover);
    expect_equal(2, indicators.ledCallCount, "Second press should call setButtonLed again");
    expect_true(!indicators.lastLedState, "Second press should turn toggle LED off");
}

void test_control_board_remote_toggle_updates_led_state()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.toggleButtonLed(controlSystem::controlBoardButtons::k_toggleDac);

    expect_equal(1, indicators.ledCallCount, "Remote toggle should update the button LED");
    expect_equal(controlSystem::controlBoardButtons::k_toggleDac, indicators.lastLedPin,
                 "Remote toggle should update the matching button LED");
    expect_true(indicators.lastLedState, "First remote toggle should turn the LED on");

    dispatcher.toggleButtonLed(controlSystem::controlBoardButtons::k_toggleDac);

    expect_equal(2, indicators.ledCallCount, "Second remote toggle should update the LED again");
    expect_true(!indicators.lastLedState, "Second remote toggle should turn the LED off");
}

void test_dynamic_toggle_action_reset_state_restores_first_press_to_on()
{
    actions::DynamicToggleAction action(CMD_RANDOM_ON, CMD_RANDOM_OFF);

    auto first = action.produce(true);
    expect_true(first != nullptr, "First press should produce an action");
    expect_equal(static_cast<uint32_t>(CMD_RANDOM_ON), static_cast<uint32_t>(first->command),
                 "First press should produce the ON command");

    auto second = action.produce(true);
    expect_true(second != nullptr, "Second press should produce an action");
    expect_equal(static_cast<uint32_t>(CMD_RANDOM_OFF), static_cast<uint32_t>(second->command),
                 "Second press should produce the OFF command");

    action.resetState();

    auto afterReset = action.produce(true);
    expect_true(afterReset != nullptr, "Press after reset should produce an action");
    expect_equal(static_cast<uint32_t>(CMD_RANDOM_ON), static_cast<uint32_t>(afterReset->command),
                 "Press after reset should return to ON command");
}

void test_dynamic_toggle_action_sync_external_state_changes_next_press_command()
{
    actions::DynamicToggleAction action(CMD_RANDOM_ON, CMD_RANDOM_OFF);

    // Simulate a remote (app) toggle that turned the feature ON without a physical press.
    action.syncExternalState(true);

    auto next = action.produce(true);
    expect_true(next != nullptr, "Press after sync should produce an action");
    expect_equal(static_cast<uint32_t>(CMD_RANDOM_OFF), static_cast<uint32_t>(next->command),
                 "Press after syncing to ON should flip to the OFF command");
}

void test_control_board_remote_toggle_syncs_registered_action_state()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction toggleAction(CMD_TOGGLE_METER_ON);
    actionMap[controlSystem::controlBoardButtons::k_toggleMeter] = {&toggleAction, controlSystem::LedPolicy::Toggle};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.toggleButtonLed(controlSystem::controlBoardButtons::k_toggleMeter);

    expect_equal(1, toggleAction.syncCallCount, "Remote toggle should sync the registered action's state");
    expect_true(toggleAction.lastSyncState, "First remote toggle should sync the action to the ON state");

    dispatcher.toggleButtonLed(controlSystem::controlBoardButtons::k_toggleMeter);

    expect_equal(2, toggleAction.syncCallCount, "Second remote toggle should sync the action again");
    expect_true(!toggleAction.lastSyncState, "Second remote toggle should sync the action to the OFF state");
}

void test_control_board_physical_press_after_remote_toggle_stays_in_sync()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    actions::DynamicToggleAction meterAction(CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF);
    actionMap[controlSystem::controlBoardButtons::k_toggleMeter] = {&meterAction, controlSystem::LedPolicy::Toggle};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    // Physical press turns the meter ON.
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_toggleMeter);
    expect_true(indicators.lastLedState, "Physical press should turn the LED on");

    // App remotely toggles the meter OFF (LED bit flips back off, action state synced).
    dispatcher.toggleButtonLed(controlSystem::controlBoardButtons::k_toggleMeter);
    expect_true(!indicators.lastLedState, "Remote toggle should turn the LED off");

    // Next physical press should continue from the synced OFF state and turn it back ON,
    // not emit a stale command based on the pre-sync internal state.
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_toggleMeter);
    expect_true(indicators.lastLedState, "Physical press after remote toggle should turn the LED back on");
    expect_equal(static_cast<uint32_t>(CMD_TOGGLE_METER_ON), static_cast<uint32_t>(responseSink.lastAction->command),
                 "Physical press after remote toggle should emit the ON command, matching the LED state");
}

void test_control_board_sleep_status_resets_toggle_led_tracking()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction coverAction(CMD_COVER_VIEW_ON);
    actionMap[controlSystem::controlBoardButtons::k_cover] = {&coverAction, controlSystem::LedPolicy::Toggle};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_cover);
    expect_true(indicators.lastLedState, "Initial toggle press should turn LED on");

    dispatcher.setBackgroundStatus(ControlBoardWorkingStatus::sleeping);
    dispatcher.setBackgroundStatus(ControlBoardWorkingStatus::Idle);

    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_cover);
    expect_true(indicators.lastLedState,
                "First toggle press after sleep reset should set LED to on state");
}

void test_control_board_out_of_range_press_keeps_existing_status_ordering()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_count);

    expect_equal(0, responseSink.callCount, "Out-of-range press should not forward a response");
    expect_equal(static_cast<size_t>(1), indicators.activityHistory.size(),
                 "Out-of-range press should preserve the current status-before-bounds-check behavior");
    expect_true(indicators.activityHistory[0] == ControlBoardWorkingStatus::doingWork,
                "Out-of-range press should still set doingWork before returning");
}

void test_control_board_rotary_uses_shared_action_slot_and_returns_to_idle()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction action(CMD_ROTARY_ACTION);
    actionMap[controlSystem::controlBoardButtons::k_rotaryEventLeft] = {&action, controlSystem::LedPolicy::None};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);
    dispatcher.handleRotaryMovement(1);

    expect_equal(1, action.callCount, "Rotary movement should execute the shared rotary action once");
    // handleRotaryMovement() deliberately inverts direction (see its implementation comment):
    // wiring makes positive direction mean "right", so isLeft (produce's argument) is false.
    expect_true(!action.lastPressedArg, "Positive rotary movement should pass false (right) to the action");
    expect_equal(1, responseSink.callCount, "Rotary movement should forward Action");
    expect_equal(static_cast<size_t>(2), indicators.activityHistory.size(), "Rotary movement should set status twice");
    expect_true(indicators.activityHistory[0] == ControlBoardWorkingStatus::doingWork,
                "Rotary movement should enter doingWork first");
    expect_true(indicators.activityHistory[1] == ControlBoardWorkingStatus::Idle,
                "Rotary movement should return to Idle afterwards");
}

void test_heartbeat_helper_handles_current_heartbeat()
{
    expect_true(controlSystem::isHeartbeatCommand(CMD_SYS_HEARTBEAT),
                "Current heartbeat should be recognized");
}

void test_heartbeat_helper_handles_legacy_heartbeat()
{
    expect_true(controlSystem::isHeartbeatCommand(controlSystem::kLegacyHeartbeatCommandId),
                "Legacy heartbeat should be recognized");
}

void test_heartbeat_helper_ignores_non_heartbeat_messages()
{
    expect_true(!controlSystem::isHeartbeatCommand(CMD_PLAY_PAUSE),
                "Non-heartbeat command should not be recognized");
}

void test_action_uart_dispatcher_routes_simple_command()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    const auto action = makeAction(CMD_PLAY_PAUSE);

    const bool handled = dispatcher.handle(*action);

    expect_true(handled, "Simple UART command should be handled");
    expect_equal(1, uartSink.commandCount, "Simple UART command should send one command");
    expect_equal(0, uartSink.messageCount, "Simple UART command should not send a structured message");
    expect_equal(std::string("Play_Pause"), uartSink.lastLogTag, "Simple UART command should use configured log tag");
    expect_equal(static_cast<uint32_t>(CMD_PLAY_PAUSE), uartSink.lastCommandId, "Simple UART command should forward command id");
}

void test_action_uart_dispatcher_routes_cover_view_message()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    const auto action = makeAction(CMD_COVER_VIEW_ON);

    const bool handled = dispatcher.handle(*action);

    expect_true(handled, "Cover view command should be handled");
    expect_equal(0, uartSink.commandCount, "Cover view should not use simple command path");
    expect_equal(1, uartSink.messageCount, "Cover view should send one structured message");
    expect_equal(std::string("Cover_View"), uartSink.lastLogTag, "Cover view should use Cover_View log tag");
    expect_equal(static_cast<uint16_t>(CMD_TOGGLE_COVER_VIEW), uartSink.lastMessage.commandId,
                 "Cover view should normalize to toggle cover command");
    expect_equal(static_cast<uint16_t>(1), uartSink.lastMessage.params[0],
                 "Cover view ON should map to parameter 1");
}

void test_action_uart_dispatcher_routes_meter_message()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    const auto action = makeAction(CMD_TOGGLE_METER_OFF);

    const bool handled = dispatcher.handle(*action);

    expect_true(handled, "Meter command should be handled");
    expect_equal(0, uartSink.commandCount, "Meter command should not use simple command path");
    expect_equal(1, uartSink.messageCount, "Meter command should send one structured message");
    expect_equal(std::string("Meter"), uartSink.lastLogTag, "Meter command should use Meter log tag");
    expect_equal(static_cast<uint16_t>(CMD_TOGGLE_METER), uartSink.lastMessage.commandId,
                 "Meter command should normalize to toggle meter command");
    expect_equal(static_cast<uint16_t>(0), uartSink.lastMessage.params[0],
                 "Meter OFF should map to parameter 0");
}

void test_action_uart_dispatcher_routes_meter_on_message()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    const auto action = makeAction(CMD_TOGGLE_METER_ON);

    const bool handled = dispatcher.handle(*action);

    expect_true(handled, "Meter ON command should be handled");
    expect_equal(0, uartSink.commandCount, "Meter ON command should not use simple command path");
    expect_equal(1, uartSink.messageCount, "Meter ON command should send one structured message");
    expect_equal(std::string("Meter"), uartSink.lastLogTag, "Meter ON command should use Meter log tag");
    expect_equal(static_cast<uint16_t>(CMD_TOGGLE_METER), uartSink.lastMessage.commandId,
                 "Meter ON command should normalize to toggle meter command");
    expect_equal(static_cast<uint16_t>(1), uartSink.lastMessage.params[0],
                 "Meter ON should map to parameter 1");
}

void test_action_uart_dispatcher_toggles_generic_meter_command()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    const auto action = makeAction(CMD_TOGGLE_METER);

    expect_true(dispatcher.handle(*action), "Generic Meter command should be handled");
    expect_equal(1, uartSink.messageCount, "First Meter toggle should send one structured message");
    expect_equal(static_cast<uint16_t>(CMD_TOGGLE_METER), uartSink.lastMessage.commandId,
                 "Generic Meter toggle should keep the normalized command");
    expect_equal(static_cast<uint16_t>(1), uartSink.lastMessage.params[0],
                 "First Meter toggle should enable the meter");

    expect_true(dispatcher.handle(*action), "Second generic Meter command should be handled");
    expect_equal(2, uartSink.messageCount, "Second Meter toggle should send another structured message");
    expect_equal(static_cast<uint16_t>(0), uartSink.lastMessage.params[0],
                 "Second Meter toggle should disable the meter");
}

void test_action_uart_dispatcher_routes_rotary_message()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    auto action = makeAction(CMD_ROTARY_ACTION);
    action->parameters[0] = 1;

    const bool handled = dispatcher.handle(*action);

    expect_true(handled, "Rotary command should be handled");
    expect_equal(0, uartSink.commandCount, "Rotary command should not use simple command path");
    expect_equal(1, uartSink.messageCount, "Rotary command should send one structured message");
    expect_equal(std::string("Rotary"), uartSink.lastLogTag, "Rotary command should use Rotary log tag");
    expect_equal(static_cast<uint16_t>(CMD_ROTARY_ACTION), uartSink.lastMessage.commandId,
                 "Rotary command should preserve command id");
    expect_equal(static_cast<uint16_t>(1), uartSink.lastMessage.params[0],
                 "Rotary command should preserve direction parameter");
}

namespace
{
    // Concrete no-op IAction: ActionUartDispatcher::handle() needs a real instance,
    // and this DTO is not part of the live firmware pipeline.
    struct NoOpAction : actions::IAction
    {
        bool requiresPowerOn() const override { return true; }
        void execute(controlSystem::ActionContext &) override {}
    };
}

void test_action_uart_dispatcher_ignores_unknown_command()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    NoOpAction action;
    action.command = CMD_NO_ACTION;
    const bool handled = dispatcher.handle(action);

    expect_true(!handled, "Unknown command should not be handled by UART dispatcher");
    expect_equal(0, uartSink.commandCount, "Unknown command should not send a command");
    expect_equal(0, uartSink.messageCount, "Unknown command should not send a message");
}

void test_power_state_transition_policy_selects_power_on_for_sleeping_states()
{
    expect_true(controlSystem::evaluatePowerTransition(ControlBoardPowerState::OFF, 0) ==
                    controlSystem::PowerTransitionAction::PowerOn,
                "OFF should transition to PowerOn");
    expect_true(controlSystem::evaluatePowerTransition(ControlBoardPowerState::SLEEP, 100) ==
                    controlSystem::PowerTransitionAction::PowerOn,
                "SLEEP should transition to PowerOn");
    expect_true(controlSystem::evaluatePowerTransition(ControlBoardPowerState::DEEPSLEEP, 100) ==
                    controlSystem::PowerTransitionAction::PowerOn,
                "DEEPSLEEP should transition to PowerOn");
}

void test_power_state_transition_policy_selects_sleep_for_short_press()
{
    expect_true(controlSystem::evaluatePowerTransition(ControlBoardPowerState::ON, 2999) ==
                    controlSystem::PowerTransitionAction::Sleep,
                "Short press while ON should transition to Sleep");
}

void test_power_state_transition_policy_selects_deep_sleep_for_long_press()
{
    expect_true(controlSystem::evaluatePowerTransition(ControlBoardPowerState::ON, 3000) ==
                    controlSystem::PowerTransitionAction::DeepSleep,
                "Threshold press while ON should transition to DeepSleep");
}

void test_power_state_transition_policy_returns_none_for_non_on_intermediate_states()
{
    expect_true(controlSystem::evaluatePowerTransition(ControlBoardPowerState::TURNING_ON, 100) ==
                    controlSystem::PowerTransitionAction::None,
                "Intermediate states should not trigger a transition");
}

void test_action_command_routing_policy_handles_pre_on_routes()
{
    expect_true(controlSystem::classifyCommand(CMD_NO_ACTION, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::None,
                "No action should short-circuit");
    expect_true(controlSystem::classifyCommand(CMD_SYS_POWER, ControlBoardPowerState::OFF) ==
                    controlSystem::ActionCommandRoute::PowerStateTransition,
                "Power command should route to power transition handling");
    expect_true(controlSystem::classifyCommand(CMD_PLAY_PAUSE, ControlBoardPowerState::OFF) ==
                    controlSystem::ActionCommandRoute::IgnoreWhileNotOn,
                "Non-power commands should be ignored while power is not ON");
}

void test_action_command_routing_policy_classifies_on_state_handlers()
{
    expect_true(controlSystem::classifyCommand(CMD_SYS_RPI_SHUTDOWN, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::System,
                "Shutdown should use the system handler");
    expect_true(controlSystem::classifyCommand(CMD_TOGGLE_DAC_ON, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::Relay,
                "DAC toggle should use the relay handler");
    expect_true(controlSystem::classifyCommand(CMD_TOGGLE_DAC, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::Relay,
                "Generic DAC toggle should use the relay handler");
    expect_true(controlSystem::classifyCommand(CMD_CYCLE_BRIGHTNESS, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::Brightness,
                "Cycle brightness should use the brightness handler");
    expect_true(controlSystem::classifyCommand(CMD_SET_BRIGHTNESS_UP, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::Brightness,
                "Brightness up should use the brightness handler");
    expect_true(controlSystem::classifyCommand(CMD_SET_BRIGHTNESS_DOWN, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::Brightness,
                "Brightness down should use the brightness handler");
    expect_true(controlSystem::classifyCommand(CMD_PLAY_PAUSE, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::UartDispatch,
                "Remaining ON-state commands should fall through to UART dispatch");
}

void test_control_board_rotary_negative_direction_passes_true_to_action()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction action(CMD_ROTARY_ACTION);
    actionMap[controlSystem::controlBoardButtons::k_rotaryEventLeft] = {&action, controlSystem::LedPolicy::None};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);
    dispatcher.handleRotaryMovement(-1);

    expect_equal(1, action.callCount, "Negative rotary movement should execute the shared action once");
    // Negative direction maps to isLeft=true per the deliberate wiring inversion.
    expect_true(action.lastPressedArg, "Negative rotary direction should pass true (left) to the action");
    expect_equal(1, responseSink.callCount, "Negative rotary movement should forward Action");
}

void test_control_board_in_range_unmapped_button_press_does_not_dispatch_response()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_playPause);

    expect_equal(0, responseSink.callCount, "In-range unmapped press should not dispatch a response");
    expect_equal(0, indicators.ledCallCount, "In-range unmapped press should not change any LED");
    expect_equal(static_cast<size_t>(1), indicators.activityHistory.size(),
                 "In-range unmapped press should still set doingWork status");
}

void test_control_board_in_range_unmapped_button_release_does_not_dispatch_response()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);
    dispatcher.handleButtonReleased(controlSystem::controlBoardButtons::k_playPause);

    expect_equal(0, responseSink.callCount, "In-range unmapped release should not dispatch a response");
    expect_equal(0, indicators.ledCallCount, "In-range unmapped release should not change any LED");
    expect_equal(static_cast<size_t>(1), indicators.activityHistory.size(),
                 "In-range unmapped release should still set background status");
}

void test_control_board_sleep_blocks_non_power_button_press_and_toggle_led_change()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction coverAction(CMD_NO_ACTION);
    actionMap[controlSystem::controlBoardButtons::k_cover] = {&coverAction, controlSystem::LedPolicy::Toggle};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.setBackgroundStatus(ControlBoardWorkingStatus::sleeping);
    // Entering sleep legitimately clears toggle LEDs via resetToggleLeds(); isolate the
    // press's own effect by comparing against the count right after that transition.
    const int ledCallsAfterSleepEntry = indicators.ledCallCount;
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_cover);

    expect_equal(0, coverAction.callCount, "Sleeping mode should not evaluate non-power button actions");
    expect_equal(0, responseSink.callCount, "Sleeping mode should not dispatch non-power button responses");
    expect_equal(ledCallsAfterSleepEntry, indicators.ledCallCount, "Sleeping mode should not change toggle LED state for non-power button");
    expect_equal(static_cast<size_t>(0), indicators.activityHistory.size(),
                 "Sleeping mode should not change activity status for blocked non-power button press");
}

void test_control_board_sleep_blocks_non_power_button_release()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction playPauseAction(CMD_NO_ACTION);
    actionMap[controlSystem::controlBoardButtons::k_playPause] = {&playPauseAction, controlSystem::LedPolicy::Momentary};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.setBackgroundStatus(ControlBoardWorkingStatus::sleeping);
    dispatcher.handleButtonReleased(controlSystem::controlBoardButtons::k_playPause);

    expect_equal(0, playPauseAction.callCount, "Sleeping mode should not evaluate non-power button release actions");
    expect_equal(0, responseSink.callCount, "Sleeping mode should not dispatch non-power button release responses");
    expect_equal(0, indicators.ledCallCount, "Sleeping mode should not update LEDs for blocked non-power button release");
    expect_equal(static_cast<size_t>(0), indicators.activityHistory.size(),
                 "Sleeping mode should not change activity status for blocked non-power button release");
}

void test_control_board_sleep_allows_power_button_action()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction powerAction(CMD_SYS_POWER);
    actionMap[controlSystem::controlBoardButtons::k_power] = {&powerAction, controlSystem::LedPolicy::None};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.setBackgroundStatus(ControlBoardWorkingStatus::sleeping);
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_power);

    expect_equal(1, powerAction.callCount, "Power button should still be processed while sleeping");
    expect_equal(1, responseSink.callCount, "Power button press while sleeping should dispatch an action");
    expect_equal(CMD_SYS_POWER, responseSink.lastAction->command,
                 "Power button press while sleeping should dispatch CMD_SYS_POWER");
    expect_equal(static_cast<size_t>(1), indicators.activityHistory.size(),
                 "Power button press while sleeping should still set activity status");
    expect_true(indicators.activityHistory[0] == ControlBoardWorkingStatus::doingWork,
                "Power button press while sleeping should set doingWork status");
}

void test_heartbeat_watchdog_does_not_time_out_before_last_rx_is_recorded()
{
    transport::uart::HeartbeatWatchdog watchdog(1000);

    expect_true(!watchdog.checkAndConsumeTimeout(5'000'000), "Watchdog should never time out before any RX is recorded");
}

void test_heartbeat_watchdog_does_not_time_out_within_the_window()
{
    transport::uart::HeartbeatWatchdog watchdog(1000);

    watchdog.notifyRx(1);

    expect_true(!watchdog.checkAndConsumeTimeout(1 + 999'000), "Watchdog should not time out at exactly the threshold");
}

void test_heartbeat_watchdog_times_out_after_the_window_elapses()
{
    transport::uart::HeartbeatWatchdog watchdog(1000);

    watchdog.notifyRx(1);

    expect_true(watchdog.checkAndConsumeTimeout(1 + 1'001'000), "Watchdog should time out once elapsed time exceeds the threshold");
}

void test_heartbeat_watchdog_does_not_refire_until_next_window_elapses()
{
    transport::uart::HeartbeatWatchdog watchdog(1000);

    watchdog.notifyRx(1);
    expect_true(watchdog.checkAndConsumeTimeout(1 + 1'001'000), "First timeout past the threshold should fire");
    expect_true(!watchdog.checkAndConsumeTimeout(1 + 1'500'000), "Timeout should not refire again before another full window elapses");
    expect_true(watchdog.checkAndConsumeTimeout(1 + 2'002'000), "Timeout should fire again once another full window elapses");
}

void test_heartbeat_watchdog_notify_rx_updates_last_rx_time()
{
    transport::uart::HeartbeatWatchdog watchdog(1000);

    watchdog.notifyRx(42);
    expect_equal(static_cast<uint64_t>(42), watchdog.getLastRxTimeUs(), "notifyRx should record the given timestamp");

    watchdog.notifyRx(100);
    expect_equal(static_cast<uint64_t>(100), watchdog.getLastRxTimeUs(), "notifyRx should overwrite the previous timestamp");
}

void test_control_board_sleep_blocks_rotary_input()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    FakeResponseSink responseSink;
    FakeIndicators indicators;
    FakeAction rotaryAction(CMD_ROTARY_ACTION);
    actionMap[controlSystem::controlBoardButtons::k_rotaryEventLeft] = {&rotaryAction, controlSystem::LedPolicy::None};

    controlSystem::ControlBoardInputDispatcher dispatcher(
        actionMap,
        [&responseSink](std::unique_ptr<actions::IAction> iaction) { responseSink.process(std::move(iaction)); },
        indicators);

    dispatcher.setBackgroundStatus(ControlBoardWorkingStatus::sleeping);
    dispatcher.handleRotaryMovement(1);

    expect_equal(0, rotaryAction.callCount, "Sleeping mode should block rotary action evaluation");
    expect_equal(0, responseSink.callCount, "Sleeping mode should block rotary response dispatch");
    expect_equal(static_cast<size_t>(0), indicators.activityHistory.size(),
                 "Sleeping mode should not change activity status for blocked rotary input");
}

void test_action_registry_populates_every_button_with_expected_policy()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    controlSystem::ControlBoardActionRegistry registry;

    registry.populate(actionMap);

    for (const auto &config : actionMap)
    {
        expect_true(config.action != nullptr, "Every registered button should have an action source");
    }

    expect_true(actionMap[controlSystem::controlBoardButtons::k_power].ledPolicy == controlSystem::LedPolicy::None,
                "Power should not have a button LED policy");
    expect_true(actionMap[controlSystem::controlBoardButtons::k_playPause].ledPolicy == controlSystem::LedPolicy::Momentary,
                "Play/pause should use a momentary LED policy");
    expect_true(actionMap[controlSystem::controlBoardButtons::k_cover].ledPolicy == controlSystem::LedPolicy::Toggle,
                "Cover should use a toggle LED policy");
    expect_true(actionMap[controlSystem::controlBoardButtons::k_rotaryEventLeft].ledPolicy == controlSystem::LedPolicy::None,
                "Rotary should not use a button LED policy");
    expect_true(actionMap[controlSystem::controlBoardButtons::k_rotaryEventLeft].action ==
                    actionMap[controlSystem::controlBoardButtons::k_rotaryEventRight].action,
                "Both rotary directions should share one action source");
}

void test_action_registry_maps_simple_toggle_and_rotary_commands()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    controlSystem::ControlBoardActionRegistry registry;
    registry.populate(actionMap);

    struct ExpectedCommand
    {
        uint8_t buttonId;
        CommandId command;
    };

    const ExpectedCommand firstPressCommands[] = {
        {controlSystem::controlBoardButtons::k_prevTrack, CMD_PREVIOUS_TRACK},
        {controlSystem::controlBoardButtons::k_nextTrack, CMD_NEXT_TRACK},
        {controlSystem::controlBoardButtons::k_skipForward, CMD_SKIP_FORWARD},
        {controlSystem::controlBoardButtons::k_skipBack, CMD_SKIP_BACK},
        {controlSystem::controlBoardButtons::k_playPause, CMD_PLAY_PAUSE},
        {controlSystem::controlBoardButtons::k_toggleDisplay, CMD_TOGGLE_DISPLAY},
        {controlSystem::controlBoardButtons::k_cover, CMD_COVER_VIEW_ON},
        {controlSystem::controlBoardButtons::k_repeat, CMD_REPEAT_ON},
        {controlSystem::controlBoardButtons::k_toggleRandom, CMD_RANDOM_ON},
        {controlSystem::controlBoardButtons::k_toggleDac, CMD_TOGGLE_DAC_ON},
        {controlSystem::controlBoardButtons::k_nextPanel, CMD_NEXT_MENU_ITEM},
        {controlSystem::controlBoardButtons::k_toggleMeter, CMD_TOGGLE_METER_ON},
        {controlSystem::controlBoardButtons::k_cycleBrightness, CMD_CYCLE_BRIGHTNESS},
    };

    for (const auto &expected : firstPressCommands)
    {
        auto action = actionMap[expected.buttonId].action->produce(true);
        expect_true(action != nullptr, "Registered button should produce an action on press");
        expect_equal(static_cast<uint16_t>(expected.command), static_cast<uint16_t>(action->command),
                     "Registered button produced an unexpected command");
    }

    auto toggleOff = actionMap[controlSystem::controlBoardButtons::k_cover].action->produce(true);
    expect_equal(static_cast<uint16_t>(CMD_COVER_VIEW_OFF), static_cast<uint16_t>(toggleOff->command),
                 "Second cover press should produce the OFF command");

    auto rotaryLeft = actionMap[controlSystem::controlBoardButtons::k_rotaryEventLeft].action->produce(true);
    auto rotaryRight = actionMap[controlSystem::controlBoardButtons::k_rotaryEventLeft].action->produce(false);
    expect_equal(static_cast<uint16_t>(CMD_ROTARY_ACTION), static_cast<uint16_t>(rotaryLeft->command),
                 "Rotary registration should produce the rotary command");
    expect_equal(static_cast<uint16_t>(0), rotaryLeft->parameters[0],
                 "Rotary left should encode parameter zero");
    expect_equal(static_cast<uint16_t>(1), rotaryRight->parameters[0],
                 "Rotary right should encode parameter one");
}

void test_action_registry_reset_states_restores_toggle_actions()
{
    controlSystem::ControlBoardInputDispatcher::ActionMap actionMap{};
    controlSystem::ControlBoardActionRegistry registry;
    registry.populate(actionMap);

    auto first = actionMap[controlSystem::controlBoardButtons::k_repeat].action->produce(true);
    expect_equal(static_cast<uint16_t>(CMD_REPEAT_ON), static_cast<uint16_t>(first->command),
                 "Repeat should start in the OFF state");

    registry.resetActionStates();

    auto afterReset = actionMap[controlSystem::controlBoardButtons::k_repeat].action->produce(true);
    expect_equal(static_cast<uint16_t>(CMD_REPEAT_ON), static_cast<uint16_t>(afterReset->command),
                 "Reset should restore the first toggle command");
}

void test_action_factory_creates_expected_action_types_and_preserves_release_time()
{
    const auto uart = controlSystem::createAction(CMD_PLAY_PAUSE);
    const auto relay = controlSystem::createAction(CMD_TOGGLE_DAC_ON);
    const auto power = controlSystem::createAction(CMD_SYS_POWER, 1234);
    const auto system = controlSystem::createAction(CMD_SYS_RPI_SHUTDOWN);
    const auto brightness = controlSystem::createAction(CMD_CYCLE_BRIGHTNESS);

    expect_true(dynamic_cast<actions::UartDispatchAction *>(uart.get()) != nullptr,
                "Simple commands should create UartDispatchAction");
    expect_true(dynamic_cast<actions::RelayAction *>(relay.get()) != nullptr,
                "Relay commands should create RelayAction");
    expect_true(dynamic_cast<actions::PowerTransitionAction *>(power.get()) != nullptr,
                "Power commands should create PowerTransitionAction");
    expect_true(dynamic_cast<actions::SystemAction *>(system.get()) != nullptr,
                "System commands should create SystemAction");
    expect_true(dynamic_cast<actions::BrightnessAction *>(brightness.get()) != nullptr,
                "Brightness commands should create BrightnessAction");
    expect_equal(static_cast<uint16_t>(1234), power->releaseTimeMillis,
                 "Power action should preserve release duration");
    expect_true(controlSystem::createAction(CMD_NO_ACTION) == nullptr,
                "No-action command should not create an action");
}

void test_command_catalog_returns_names_and_simple_tags()
{
    expect_equal(std::string("Play_Pause"),
                 std::string(controlSystem::getCommandNameById(CMD_PLAY_PAUSE)),
                 "Known command should return its catalog name");
    expect_equal(std::string("Play_Pause"),
                 std::string(controlSystem::getSimpleCommandLogTag(CMD_PLAY_PAUSE)),
                 "Simple command should return its log tag");
    expect_true(controlSystem::getSimpleCommandLogTag(CMD_TOGGLE_METER) == nullptr,
                "Structured command should not have a simple log tag");
    expect_equal(std::string("UNKNOWN"),
                 std::string(controlSystem::getCommandNameById(static_cast<CommandId>(0xFFFF))),
                 "Unknown command should return UNKNOWN");
    expect_true(controlSystem::getSimpleCommandLogTag(static_cast<CommandId>(0xFFFF)) == nullptr,
                "Unknown command should not have a log tag");
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
        {"test_serialize_message_playlist_cmd_writes_variable_length_payload", test_serialize_message_playlist_cmd_writes_variable_length_payload},
        {"test_deserialize_message_now_playing_payload", test_deserialize_message_now_playing_payload},
        {"test_deserialize_message_track_progress_payload", test_deserialize_message_track_progress_payload},
        {"test_deserialize_message_library_entry_payload", test_deserialize_message_library_entry_payload},
        {"test_deserialize_message_library_entry_payload_with_art_hash", test_deserialize_message_library_entry_payload_with_art_hash},
        {"test_deserialize_message_library_entry_payload_max_lengths", test_deserialize_message_library_entry_payload_max_lengths},
        {"test_deserialize_message_playlist_result_payload", test_deserialize_message_playlist_result_payload},
        {"test_deserialize_message_playlist_result_failure_payload", test_deserialize_message_playlist_result_failure_payload},
        {"test_serialize_message_clamps_playlist_name_to_protocol_limit", test_serialize_message_clamps_playlist_name_to_protocol_limit},
        {"test_deserialize_message_rejects_invalid_fixed_payload_lengths", test_deserialize_message_rejects_invalid_fixed_payload_lengths},
        {"test_deserialize_message_ignores_incomplete_standard_parameter_pair", test_deserialize_message_ignores_incomplete_standard_parameter_pair},
        {"test_control_board_button_press_dispatches_action_and_led", test_control_board_button_press_dispatches_action_and_led},
        {"test_control_board_momentary_button_release_turns_led_off", test_control_board_momentary_button_release_turns_led_off},
        {"test_control_board_toggle_button_press_flips_led_state", test_control_board_toggle_button_press_flips_led_state},
        {"test_control_board_remote_toggle_updates_led_state", test_control_board_remote_toggle_updates_led_state},
        {"test_dynamic_toggle_action_reset_state_restores_first_press_to_on", test_dynamic_toggle_action_reset_state_restores_first_press_to_on},
        {"test_dynamic_toggle_action_sync_external_state_changes_next_press_command", test_dynamic_toggle_action_sync_external_state_changes_next_press_command},
        {"test_control_board_remote_toggle_syncs_registered_action_state", test_control_board_remote_toggle_syncs_registered_action_state},
        {"test_control_board_physical_press_after_remote_toggle_stays_in_sync", test_control_board_physical_press_after_remote_toggle_stays_in_sync},
        {"test_control_board_sleep_status_resets_toggle_led_tracking", test_control_board_sleep_status_resets_toggle_led_tracking},
        {"test_control_board_out_of_range_press_keeps_existing_status_ordering", test_control_board_out_of_range_press_keeps_existing_status_ordering},
        {"test_control_board_rotary_uses_shared_action_slot_and_returns_to_idle", test_control_board_rotary_uses_shared_action_slot_and_returns_to_idle},
        {"test_heartbeat_helper_handles_current_heartbeat", test_heartbeat_helper_handles_current_heartbeat},
        {"test_heartbeat_helper_handles_legacy_heartbeat", test_heartbeat_helper_handles_legacy_heartbeat},
        {"test_heartbeat_helper_ignores_non_heartbeat_messages", test_heartbeat_helper_ignores_non_heartbeat_messages},
        {"test_action_uart_dispatcher_routes_simple_command", test_action_uart_dispatcher_routes_simple_command},
        {"test_action_uart_dispatcher_routes_cover_view_message", test_action_uart_dispatcher_routes_cover_view_message},
        {"test_action_uart_dispatcher_routes_meter_message", test_action_uart_dispatcher_routes_meter_message},
        {"test_action_uart_dispatcher_routes_meter_on_message", test_action_uart_dispatcher_routes_meter_on_message},
        {"test_action_uart_dispatcher_toggles_generic_meter_command", test_action_uart_dispatcher_toggles_generic_meter_command},
        {"test_action_uart_dispatcher_routes_rotary_message", test_action_uart_dispatcher_routes_rotary_message},
        {"test_action_uart_dispatcher_ignores_unknown_command", test_action_uart_dispatcher_ignores_unknown_command},
        {"test_power_state_transition_policy_selects_power_on_for_sleeping_states", test_power_state_transition_policy_selects_power_on_for_sleeping_states},
        {"test_power_state_transition_policy_selects_sleep_for_short_press", test_power_state_transition_policy_selects_sleep_for_short_press},
        {"test_power_state_transition_policy_selects_deep_sleep_for_long_press", test_power_state_transition_policy_selects_deep_sleep_for_long_press},
        {"test_power_state_transition_policy_returns_none_for_non_on_intermediate_states", test_power_state_transition_policy_returns_none_for_non_on_intermediate_states},
        {"test_action_command_routing_policy_handles_pre_on_routes", test_action_command_routing_policy_handles_pre_on_routes},
        {"test_action_command_routing_policy_classifies_on_state_handlers", test_action_command_routing_policy_classifies_on_state_handlers},
        {"test_control_board_rotary_negative_direction_passes_true_to_action", test_control_board_rotary_negative_direction_passes_true_to_action},
        {"test_control_board_in_range_unmapped_button_press_does_not_dispatch_response", test_control_board_in_range_unmapped_button_press_does_not_dispatch_response},
        {"test_control_board_in_range_unmapped_button_release_does_not_dispatch_response", test_control_board_in_range_unmapped_button_release_does_not_dispatch_response},
        {"test_control_board_sleep_blocks_non_power_button_press_and_toggle_led_change", test_control_board_sleep_blocks_non_power_button_press_and_toggle_led_change},
        {"test_control_board_sleep_blocks_non_power_button_release", test_control_board_sleep_blocks_non_power_button_release},
        {"test_control_board_sleep_allows_power_button_action", test_control_board_sleep_allows_power_button_action},
        {"test_control_board_sleep_blocks_rotary_input", test_control_board_sleep_blocks_rotary_input},
        {"test_heartbeat_watchdog_does_not_time_out_before_last_rx_is_recorded", test_heartbeat_watchdog_does_not_time_out_before_last_rx_is_recorded},
        {"test_heartbeat_watchdog_does_not_time_out_within_the_window", test_heartbeat_watchdog_does_not_time_out_within_the_window},
        {"test_heartbeat_watchdog_times_out_after_the_window_elapses", test_heartbeat_watchdog_times_out_after_the_window_elapses},
        {"test_heartbeat_watchdog_does_not_refire_until_next_window_elapses", test_heartbeat_watchdog_does_not_refire_until_next_window_elapses},
        {"test_heartbeat_watchdog_notify_rx_updates_last_rx_time", test_heartbeat_watchdog_notify_rx_updates_last_rx_time},
        {"test_action_registry_populates_every_button_with_expected_policy", test_action_registry_populates_every_button_with_expected_policy},
        {"test_action_registry_maps_simple_toggle_and_rotary_commands", test_action_registry_maps_simple_toggle_and_rotary_commands},
        {"test_action_registry_reset_states_restores_toggle_actions", test_action_registry_reset_states_restores_toggle_actions},
        {"test_action_factory_creates_expected_action_types_and_preserves_release_time", test_action_factory_creates_expected_action_types_and_preserves_release_time},
        {"test_command_catalog_returns_names_and_simple_tags", test_command_catalog_returns_names_and_simple_tags},
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
