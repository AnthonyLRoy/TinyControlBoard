#include <unity.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

// Expose private members for white-box testing
#define private public
#include "spiLedDriver.hpp"
#undef private

using indicators::SpiLedDriver;

// Use the same pins as the real board so the tests exercise the real config
static constexpr spi_host_device_t k_testSpiHost  = SPI2_HOST;
static constexpr gpio_num_t        k_testMosi      = GPIO_NUM_7;
static constexpr gpio_num_t        k_testClk       = GPIO_NUM_6;
static constexpr gpio_num_t        k_testLatch      = GPIO_NUM_5;

void setUp() {}
void tearDown() {}

// ------------------------------------------------------------------- tests ---

void test_constructor_initial_state()
{
    SpiLedDriver driver(k_testSpiHost, k_testMosi, k_testClk, k_testLatch);

    TEST_ASSERT_FALSE(driver.m_started);
    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.m_ledBitState);
}

// ---- setAllLeds() — early-return path (driver not yet initialised) ---------

void test_set_all_leds_true_before_init_does_not_modify_bit_state()
{
    SpiLedDriver driver(k_testSpiHost, k_testMosi, k_testClk, k_testLatch);

    driver.setAllLeds(true); // m_started == false → returns early

    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.m_ledBitState);
}

void test_set_all_leds_false_before_init_does_not_modify_bit_state()
{
    SpiLedDriver driver(k_testSpiHost, k_testMosi, k_testClk, k_testLatch);
    driver.m_ledBitState = 0xABCD; // seed a non-zero value

    driver.setAllLeds(false); // m_started == false → returns early

    TEST_ASSERT_EQUAL_UINT16(0xABCD, driver.m_ledBitState); // unchanged
}

// ---- setLed() — early-return path (driver not yet initialised) ------------

void test_set_led_before_init_does_not_modify_bit_state()
{
    SpiLedDriver driver(k_testSpiHost, k_testMosi, k_testClk, k_testLatch);

    driver.setLed(0, true); // m_started == false → returns early

    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.m_ledBitState);
}

// ---- setLed() — out-of-range guard (mock m_started without calling init) ---

void test_set_led_out_of_range_does_not_modify_bit_state()
{
    SpiLedDriver driver(k_testSpiHost, k_testMosi, k_testClk, k_testLatch);
    // Bypass init by forcing the started flag — this avoids touching hardware
    // while still exercising the bounds-check branch.
    driver.m_started = true;

    driver.setLed(16, true); // ledIndex >= LED_COUNT (16) → returns early

    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.m_ledBitState);
}

void test_set_led_max_valid_index_boundary()
{
    SpiLedDriver driver(k_testSpiHost, k_testMosi, k_testClk, k_testLatch);
    // LED_COUNT - 1 == 15 is the last valid index.
    // With m_started == false the early-return fires, but we verify no crash.
    driver.setLed(15, true);
    driver.setLed(16, true);

    // Both should leave bit state at 0 (not started, early return)
    TEST_ASSERT_EQUAL_UINT16(0x0000, driver.m_ledBitState);
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
