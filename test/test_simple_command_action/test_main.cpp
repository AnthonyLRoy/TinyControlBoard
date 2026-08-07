#include <unity.h>

#include "input/actions/actionTemplates.hpp"

using actions::SimpleCommandAction;

void test_simple_command_action_returns_configured_command_when_pressed()
{
    SimpleCommandAction action(CMD_PLAY_PAUSE);

    const auto iaction = action.produce(true);

    TEST_ASSERT_NOT_NULL(iaction.get());
    TEST_ASSERT_EQUAL(CMD_PLAY_PAUSE, iaction->command);
    TEST_ASSERT_FALSE(iaction->isActive);
    TEST_ASSERT_EQUAL_UINT16(0, iaction->releaseTimeMillis);
}

void test_simple_command_action_returns_no_action_when_released()
{
    SimpleCommandAction action(CMD_PLAY_PAUSE);

    const auto iaction = action.produce(false);

    TEST_ASSERT_NULL(iaction.get());
}

extern "C" void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_simple_command_action_returns_configured_command_when_pressed);
    RUN_TEST(test_simple_command_action_returns_no_action_when_released);
    UNITY_END();
}