#include <unity.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

// Expose private members for white-box testing
#define private public
#include "spiLedDriver.hpp"
#undef private

using indicators::SpiLedDriver;

// Use the same pins as the real board so the tests exercise the real config
static constexpr spi_host_device_t kTestSpiHost  = SPI2_HOST;
static constexpr gpio_num_t        kTestMosi      = GPIO_NUM_7;
static constexpr gpio_num_t        kTestClk       = GPIO_NUM_6;
static constexpr gpio_num_t        kTestLatch      = GPIO_NUM_5;

void setUp() {}
void tearDown() {}

// ------------------------------------------------------------------- tests ---

void test_constructor_initial_state()
{
    SpiLedDriver driver(kTestSpiHost, kTestMosi, kTestClk, kTestLatch);

    TEST_ASSERT_FALSE(driver.mStarted);
    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.mLedBitState);
}

// ---- setAllLeds() — early-return path (driver not yet initialised) ---------

void test_set_all_leds_true_before_init_does_not_modify_bit_state()
{
    SpiLedDriver driver(kTestSpiHost, kTestMosi, kTestClk, kTestLatch);

    driver.setAllLeds(true); // mStarted == false → returns early

    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.mLedBitState);
}

void test_set_all_leds_false_before_init_does_not_modify_bit_state()
{
    SpiLedDriver driver(kTestSpiHost, kTestMosi, kTestClk, kTestLatch);
    driver.mLedBitState = 0xABCD; // seed a non-zero value

    driver.setAllLeds(false); // mStarted == false → returns early

    TEST_ASSERT_EQUAL_UINT16(0xABCD, driver.mLedBitState); // unchanged
}

// ---- setLed() — early-return path (driver not yet initialised) ------------

void test_set_led_before_init_does_not_modify_bit_state()
{
    SpiLedDriver driver(kTestSpiHost, kTestMosi, kTestClk, kTestLatch);

    driver.setLed(0, true); // mStarted == false → returns early

    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.mLedBitState);
}

// ---- setLed() — out-of-range guard (mock mStarted without calling init) ---

void test_set_led_out_of_range_does_not_modify_bit_state()
{
    SpiLedDriver driver(kTestSpiHost, kTestMosi, kTestClk, kTestLatch);
    // Bypass init by forcing the started flag — this avoids touching hardware
    // while still exercising the bounds-check branch.
    driver.mStarted = true;

    driver.setLed(16, true); // ledIndex >= LED_COUNT (16) → returns early

    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.mLedBitState);
}

void test_set_led_max_valid_index_boundary()
{
    SpiLedDriver driver(kTestSpiHost, kTestMosi, kTestClk, kTestLatch);
    // LED_COUNT - 1 == 15 is the last valid index.
    // With mStarted == false the early-return fires, but we verify no crash.
    driver.setLed(15, true);
    driver.setLed(16, true);

    // Both should leave bit state at 0 (not started, early return)
    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.mLedBitState);
}

// ---- LED_COUNT constant ----------------------------------------------------

void test_led_count_is_sixteen()
{
    // LED_COUNT is constexpr — just confirm the compile-time constant matches
    // the expected 16-channel hardware.
    TEST_ASSERT_EQUAL(16, SpiLedDriver::LED_COUNT);
}

// ------------------------------------------------------------------ entry ---

extern "C" void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_constructor_initial_state);
    RUN_TEST(test_set_all_leds_true_before_init_does_not_modify_bit_state);
    RUN_TEST(test_set_all_leds_false_before_init_does_not_modify_bit_state);
    RUN_TEST(test_set_led_before_init_does_not_modify_bit_state);
    RUN_TEST(test_set_led_out_of_range_does_not_modify_bit_state);
    RUN_TEST(test_set_led_max_valid_index_boundary);
    RUN_TEST(test_led_count_is_sixteen);
    UNITY_END();
}
