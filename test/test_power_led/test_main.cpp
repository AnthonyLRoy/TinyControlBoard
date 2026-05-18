#include <unity.h>

#define private public
#include "powerLed.hpp"
#undef private

using indicators::PowerLed;

void test_power_led_constructor_sets_expected_defaults()
{
    PowerLed led(GPIO_NUM_1, LEDC_CHANNEL_0, GPIO_NUM_2, LEDC_CHANNEL_1);

    TEST_ASSERT_EQUAL(GPIO_NUM_1, led.m_activePin);
    TEST_ASSERT_EQUAL(LEDC_CHANNEL_0, led.m_activeChannel);
    TEST_ASSERT_EQUAL(GPIO_NUM_2, led.m_standbyPin);
    TEST_ASSERT_EQUAL(LEDC_CHANNEL_1, led.m_standbyChannel);

    TEST_ASSERT_NULL(led.mp_updateTimer);
    TEST_ASSERT_EQUAL(ControlBoardPowerState::OFF, led.getState());
    TEST_ASSERT_FALSE(led.m_activeFlash);
    TEST_ASSERT_FALSE(led.m_standbyFlash);
    TEST_ASSERT_FALSE(led.m_activeBreathing);
    TEST_ASSERT_FALSE(led.m_standbyBreathing);
    TEST_ASSERT_FALSE(led.m_activeBlip);
    TEST_ASSERT_FALSE(led.m_standbyBlip);
}

void test_power_led_set_brightness_clamps_and_scales()
{
    PowerLed led(GPIO_NUM_1, LEDC_CHANNEL_0, GPIO_NUM_2, LEDC_CHANNEL_1);

    led.setBrightness(-25);
    TEST_ASSERT_EQUAL(0, led.m_dutyCycle);

    led.setBrightness(50);
    TEST_ASSERT_EQUAL(2048, led.m_dutyCycle);

    led.setBrightness(150);
    TEST_ASSERT_EQUAL(4055, led.m_dutyCycle); // 99 * 4096 / 100
}

extern "C" void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_led_constructor_sets_expected_defaults);
    RUN_TEST(test_power_led_set_brightness_clamps_and_scales);
    UNITY_END();
}
