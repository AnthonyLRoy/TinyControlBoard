#include <unity.h>

#include "input/actions/SimpleCommandAction.hpp"

using actions::ActionResponse;
using actions::SimpleCommandAction;

void test_simple_command_action_returns_configured_command_when_pressed()
{
    SimpleCommandAction action(CMD_PLAY_PAUSE);

    const ActionResponse response = action.execute(true);

    TEST_ASSERT_EQUAL(CMD_PLAY_PAUSE, response.command);
    TEST_ASSERT_FALSE(response.isActive);
    TEST_ASSERT_FALSE(response.keepLedActive);
    TEST_ASSERT_EQUAL_UINT16(0, response.releaseTimeMillis);
}

void test_simple_command_action_returns_no_action_when_released()
{
    SimpleCommandAction action(CMD_PLAY_PAUSE);

    const ActionResponse response = action.execute(false);

    TEST_ASSERT_EQUAL(CMD_NO_ACTION, response.command);
    TEST_ASSERT_FALSE(response.isActive);
    TEST_ASSERT_FALSE(response.keepLedActive);
    TEST_ASSERT_EQUAL_UINT16(0, response.releaseTimeMillis);
}

extern "C" void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_simple_command_action_returns_configured_command_when_pressed);
    RUN_TEST(test_simple_command_action_returns_no_action_when_released);
    UNITY_END();
}