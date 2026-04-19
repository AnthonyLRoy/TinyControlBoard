#include <unity.h>

#define private public
#include "powerLed.hpp"
#undef private

using indicators::PowerLed;

void test_power_led_constructor_sets_expected_defaults()
{
    PowerLed led(GPIO_NUM_1, LEDC_CHANNEL_0, GPIO_NUM_2, LEDC_CHANNEL_1);

    TEST_ASSERT_EQUAL(GPIO_NUM_1, led.mActivePin);
    TEST_ASSERT_EQUAL(LEDC_CHANNEL_0, led.mActiveChannel);
    TEST_ASSERT_EQUAL(GPIO_NUM_2, led.mStandbyPin);
    TEST_ASSERT_EQUAL(LEDC_CHANNEL_1, led.mStandbyChannel);

    TEST_ASSERT_NULL(led.mpUpdateTimer);
    TEST_ASSERT_EQUAL(ControlBoardPowerState::OFF, led.getState());
    TEST_ASSERT_FALSE(led.mActiveFlash);
    TEST_ASSERT_FALSE(led.mStandbyFlash);
    TEST_ASSERT_FALSE(led.mActiveBreathing);
    TEST_ASSERT_FALSE(led.mStandbyBreathing);
    TEST_ASSERT_FALSE(led.mActiveBlip);
    TEST_ASSERT_FALSE(led.mStandbyBlip);
}

void test_power_led_set_brightness_clamps_and_scales()
{
    PowerLed led(GPIO_NUM_1, LEDC_CHANNEL_0, GPIO_NUM_2, LEDC_CHANNEL_1);

    led.setBrightness(-25);
    TEST_ASSERT_EQUAL(0, led.mDutyCycle);

    led.setBrightness(50);
    TEST_ASSERT_EQUAL(2048, led.mDutyCycle);

    led.setBrightness(150);
    TEST_ASSERT_EQUAL(4055, led.mDutyCycle); // 99 * 4096 / 100
}

extern "C" void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_led_constructor_sets_expected_defaults);
    RUN_TEST(test_power_led_set_brightness_clamps_and_scales);
    UNITY_END();
}
