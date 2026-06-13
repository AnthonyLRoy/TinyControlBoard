#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "indicators/activityStatus.hpp"
#include "input/actions/actionsResponse.hpp"
#include "input/actions/actionTemplates.hpp"
#include "app/ActionCommandCatalog.hpp"
#include "app/ActionCommandRoutingPolicy.hpp"
#include "app/ActionFactory.hpp"
#include "app/ActionUartDispatcher.hpp"
#include "app/ControlBoardButtonIds.hpp"
#include "app/ControlBoardInputDispatcher.hpp"
#include "power/DelayedBootRecoveryState.hpp"
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

    int callCount = 0;
    bool lastPressedArg = false;

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
    expect_true(action.lastPressedArg, "Positive rotary movement should pass true to the action");
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

void test_delayed_boot_recovery_arms_after_timeout()
{
    controlSystem::DelayedBootRecoveryState recovery;

    expect_true(!recovery.isPending(), "Recovery should start inactive");

    recovery.markBootTimedOut();

    expect_true(recovery.isPending(), "Timeout should arm delayed boot recovery");
}

void test_delayed_boot_recovery_completes_once_for_recoverable_states()
{
    controlSystem::DelayedBootRecoveryState recovery;

    recovery.markBootTimedOut();
    expect_true(recovery.consumeIfRecoverableState(ControlBoardPowerState::SLEEP),
                "Sleep state should allow a delayed heartbeat to complete boot");
    expect_true(!recovery.isPending(), "Successful completion should clear the pending flag");
    expect_true(!recovery.consumeIfRecoverableState(ControlBoardPowerState::SLEEP),
                "Recovery should only complete once per timeout");

    recovery.markBootTimedOut();
    expect_true(recovery.consumeIfRecoverableState(ControlBoardPowerState::TURNING_ON),
                "Turning-on state should also allow delayed boot completion");
}

void test_delayed_boot_recovery_ignores_unrecoverable_states_and_clear()
{
    controlSystem::DelayedBootRecoveryState recovery;

    recovery.markBootTimedOut();
    expect_true(!recovery.consumeIfRecoverableState(ControlBoardPowerState::OFF),
                "Off state should not consume delayed boot recovery");
    expect_true(recovery.isPending(), "Unrecoverable states should leave recovery armed");

    recovery.clear();

    expect_true(!recovery.isPending(), "Clear should disarm delayed boot recovery");
    expect_true(!recovery.consumeIfRecoverableState(ControlBoardPowerState::SLEEP),
                "No recovery should complete once the pending flag is cleared");
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

void test_action_uart_dispatcher_ignores_unknown_command()
{
    FakeUartCommandSink uartSink;
    controlSystem::ActionUartDispatcher dispatcher(uartSink);
    actions::Action action;
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

void test_action_command_catalog_centralizes_toggle_specs()
{
    const auto *coverSpec = controlSystem::findToggleCommandSpecBySemanticCommand(CMD_TOGGLE_COVER_VIEW);
    expect_true(coverSpec != nullptr, "Cover view toggle should have a shared command spec");
    expect_equal(static_cast<uint16_t>(CMD_COVER_VIEW_ON), static_cast<uint16_t>(coverSpec->onCommand),
                 "Cover view shared spec should define the ON command");
    expect_equal(static_cast<uint16_t>(CMD_COVER_VIEW_OFF), static_cast<uint16_t>(coverSpec->offCommand),
                 "Cover view shared spec should define the OFF command");
    expect_true(controlSystem::classifyCommand(CMD_COVER_VIEW_ON, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::UartDispatch,
                "Cover view ON should route through UART dispatch");

    const auto *dacSpec = controlSystem::findToggleCommandSpecBySemanticCommand(CMD_TOGGLE_DAC);
    expect_true(dacSpec != nullptr, "DAC toggle should have a shared command spec");
    expect_equal(static_cast<uint16_t>(CMD_TOGGLE_DAC_ON), static_cast<uint16_t>(dacSpec->onCommand),
                 "DAC shared spec should define the ON command");
    expect_equal(static_cast<uint16_t>(CMD_TOGGLE_DAC_OFF), static_cast<uint16_t>(dacSpec->offCommand),
                 "DAC shared spec should define the OFF command");
    expect_true(controlSystem::classifyCommand(CMD_TOGGLE_DAC_OFF, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::Relay,
                "DAC OFF should route through the relay handler");
}

void test_power_state_transition_policy_reports_transitional_indicator_states()
{
    expect_true(controlSystem::getTransitionEntryState(controlSystem::PowerTransitionAction::PowerOn,
                                                       ControlBoardPowerState::OFF) ==
                    ControlBoardPowerState::TURNING_ON,
                "Power-on transitions should enter TURNING_ON");
    expect_true(controlSystem::getTransitionEntryState(controlSystem::PowerTransitionAction::Sleep,
                                                       ControlBoardPowerState::ON) ==
                    ControlBoardPowerState::SHUTTING_DOWN,
                "Sleep transitions should first enter SHUTTING_DOWN");
    expect_true(controlSystem::getTransitionEntryState(controlSystem::PowerTransitionAction::DeepSleep,
                                                       ControlBoardPowerState::ON) ==
                    ControlBoardPowerState::SHUTTING_DOWN,
                "Deep-sleep transitions should first enter SHUTTING_DOWN");
    expect_true(controlSystem::getPostShutdownTransitionState(controlSystem::PowerTransitionAction::Sleep,
                                                              ControlBoardPowerState::ON) ==
                    ControlBoardPowerState::GOING_TO_SLEEP,
                "Sleep transitions should enter GOING_TO_SLEEP after Pi shutdown");
    expect_true(controlSystem::getPostShutdownTransitionState(controlSystem::PowerTransitionAction::DeepSleep,
                                                              ControlBoardPowerState::ON) ==
                    ControlBoardPowerState::GOING_INTO_DEEP_SLEEP,
                "Deep-sleep transitions should enter GOING_INTO_DEEP_SLEEP after Pi shutdown");
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
    expect_true(controlSystem::classifyCommand(CMD_CYCLE_BRIGHTNESS, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::Brightness,
                "Cycle brightness should use the brightness handler");
    expect_true(controlSystem::classifyCommand(CMD_PLAY_PAUSE, ControlBoardPowerState::ON) ==
                    controlSystem::ActionCommandRoute::UartDispatch,
                "Remaining ON-state commands should fall through to UART dispatch");
}

void test_control_board_rotary_negative_direction_passes_false_to_action()
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
    expect_true(!action.lastPressedArg, "Negative rotary direction should pass false to the action");
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
    dispatcher.handleButtonPressed(controlSystem::controlBoardButtons::k_cover);

    expect_equal(0, coverAction.callCount, "Sleeping mode should not evaluate non-power button actions");
    expect_equal(0, responseSink.callCount, "Sleeping mode should not dispatch non-power button responses");
    expect_equal(0, indicators.ledCallCount, "Sleeping mode should not change toggle LED state for non-power button");
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
        {"test_control_board_button_press_dispatches_action_and_led", test_control_board_button_press_dispatches_action_and_led},
        {"test_control_board_momentary_button_release_turns_led_off", test_control_board_momentary_button_release_turns_led_off},
        {"test_control_board_toggle_button_press_flips_led_state", test_control_board_toggle_button_press_flips_led_state},
        {"test_control_board_out_of_range_press_keeps_existing_status_ordering", test_control_board_out_of_range_press_keeps_existing_status_ordering},
        {"test_control_board_rotary_uses_shared_action_slot_and_returns_to_idle", test_control_board_rotary_uses_shared_action_slot_and_returns_to_idle},
        {"test_heartbeat_helper_handles_current_heartbeat", test_heartbeat_helper_handles_current_heartbeat},
        {"test_heartbeat_helper_handles_legacy_heartbeat", test_heartbeat_helper_handles_legacy_heartbeat},
        {"test_heartbeat_helper_ignores_non_heartbeat_messages", test_heartbeat_helper_ignores_non_heartbeat_messages},
        {"test_delayed_boot_recovery_arms_after_timeout", test_delayed_boot_recovery_arms_after_timeout},
        {"test_delayed_boot_recovery_completes_once_for_recoverable_states", test_delayed_boot_recovery_completes_once_for_recoverable_states},
        {"test_delayed_boot_recovery_ignores_unrecoverable_states_and_clear", test_delayed_boot_recovery_ignores_unrecoverable_states_and_clear},
        {"test_action_uart_dispatcher_routes_simple_command", test_action_uart_dispatcher_routes_simple_command},
        {"test_action_uart_dispatcher_routes_cover_view_message", test_action_uart_dispatcher_routes_cover_view_message},
        {"test_action_uart_dispatcher_routes_meter_message", test_action_uart_dispatcher_routes_meter_message},
        {"test_action_uart_dispatcher_routes_meter_on_message", test_action_uart_dispatcher_routes_meter_on_message},
        {"test_action_uart_dispatcher_routes_rotary_message", test_action_uart_dispatcher_routes_rotary_message},
        {"test_action_uart_dispatcher_ignores_unknown_command", test_action_uart_dispatcher_ignores_unknown_command},
        {"test_power_state_transition_policy_selects_power_on_for_sleeping_states", test_power_state_transition_policy_selects_power_on_for_sleeping_states},
        {"test_power_state_transition_policy_selects_sleep_for_short_press", test_power_state_transition_policy_selects_sleep_for_short_press},
        {"test_power_state_transition_policy_selects_deep_sleep_for_long_press", test_power_state_transition_policy_selects_deep_sleep_for_long_press},
        {"test_power_state_transition_policy_returns_none_for_non_on_intermediate_states", test_power_state_transition_policy_returns_none_for_non_on_intermediate_states},
        {"test_action_command_catalog_centralizes_toggle_specs", test_action_command_catalog_centralizes_toggle_specs},
        {"test_power_state_transition_policy_reports_transitional_indicator_states", test_power_state_transition_policy_reports_transitional_indicator_states},
        {"test_action_command_routing_policy_handles_pre_on_routes", test_action_command_routing_policy_handles_pre_on_routes},
        {"test_action_command_routing_policy_classifies_on_state_handlers", test_action_command_routing_policy_classifies_on_state_handlers},
        {"test_control_board_rotary_negative_direction_passes_false_to_action", test_control_board_rotary_negative_direction_passes_false_to_action},
        {"test_control_board_in_range_unmapped_button_press_does_not_dispatch_response", test_control_board_in_range_unmapped_button_press_does_not_dispatch_response},
        {"test_control_board_in_range_unmapped_button_release_does_not_dispatch_response", test_control_board_in_range_unmapped_button_release_does_not_dispatch_response},
        {"test_control_board_sleep_blocks_non_power_button_press_and_toggle_led_change", test_control_board_sleep_blocks_non_power_button_press_and_toggle_led_change},
        {"test_control_board_sleep_blocks_non_power_button_release", test_control_board_sleep_blocks_non_power_button_release},
        {"test_control_board_sleep_allows_power_button_action", test_control_board_sleep_allows_power_button_action},
        {"test_control_board_sleep_blocks_rotary_input", test_control_board_sleep_blocks_rotary_input},
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
