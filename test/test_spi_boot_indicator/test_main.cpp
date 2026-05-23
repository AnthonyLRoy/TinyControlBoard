#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Expose private members for white-box testing
#define private public
#include "SpiBootIndicator.hpp"
#undef private

using indicators::SpiBootIndicator;

void setUp() {}
void tearDown() {}

// ------------------------------------------------------------------ helpers --

// Give the flash task enough time to wake from its current vTaskDelay and
// self-delete after notifySuccess() has been called.
static void stopAndDrain(SpiBootIndicator &ind)
{
    ind.notifySuccess();
    // Worst case: task is sleeping for the full boot half-period (500 ms).
    // Add 100 ms margin for scheduling jitter.
    vTaskDelay(pdMS_TO_TICKS(600));
}

// ------------------------------------------------------------------- tests ---

void test_initial_state_is_idle()
{
    SpiBootIndicator indicator;

    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Idle, indicator.m_state);
    TEST_ASSERT_NULL(indicator.m_task);
    TEST_ASSERT_FALSE(indicator.m_stop);
}

void test_start_waiting_sets_booting_state_and_creates_task()
{
    SpiBootIndicator indicator;
    indicator.startWaiting();

    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Booting, indicator.m_state);
    TEST_ASSERT_NOT_NULL(indicator.m_task);
    TEST_ASSERT_FALSE(indicator.m_stop);

    stopAndDrain(indicator);
}

void test_start_waiting_is_idempotent_when_already_running()
{
    SpiBootIndicator indicator;
    indicator.startWaiting();
    TaskHandle_t firstTask = indicator.m_task;

    indicator.startWaiting(); // second call must be a no-op

    TEST_ASSERT_EQUAL_PTR(firstTask, indicator.m_task);

    stopAndDrain(indicator);
}

void test_notify_success_sets_stop_flag()
{
    SpiBootIndicator indicator;
    indicator.startWaiting();

    indicator.notifySuccess();

    TEST_ASSERT_TRUE(indicator.m_stop);
    vTaskDelay(pdMS_TO_TICKS(600)); // let the task finish
}

void test_notify_success_on_idle_indicator_is_safe_noop()
{
    SpiBootIndicator indicator;

    indicator.notifySuccess(); // no task running — must not crash

    TEST_ASSERT_NULL(indicator.m_task);
    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Idle, indicator.m_state);
}

void test_notify_failure_on_idle_starts_task_in_failed_state()
{
    SpiBootIndicator indicator;
    indicator.notifyFailure();

    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Failed, indicator.m_state);
    TEST_ASSERT_NOT_NULL(indicator.m_task);

    stopAndDrain(indicator);
}

void test_notify_failure_during_booting_switches_to_failed_state()
{
    SpiBootIndicator indicator;
    indicator.startWaiting();
    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Booting, indicator.m_state);

    indicator.notifyFailure();

    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Failed, indicator.m_state);
    TEST_ASSERT_NOT_NULL(indicator.m_task); // same task, still running

    stopAndDrain(indicator);
}

void test_notify_failure_on_already_failed_indicator_is_idempotent()
{
    SpiBootIndicator indicator;
    indicator.notifyFailure();
    TaskHandle_t firstTask = indicator.m_task;

    indicator.notifyFailure(); // repeat call

    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Failed, indicator.m_state);
    TEST_ASSERT_EQUAL_PTR(firstTask, indicator.m_task);

    stopAndDrain(indicator);
}

void test_task_self_clears_after_success_is_signalled()
{
    SpiBootIndicator indicator;
    indicator.startWaiting();

    indicator.notifySuccess();
    vTaskDelay(pdMS_TO_TICKS(600)); // allow task to wake and delete itself

    TEST_ASSERT_NULL(indicator.m_task);
    TEST_ASSERT_EQUAL(SpiBootIndicator::State::Idle, indicator.m_state);
}

// ------------------------------------------------------------------ entry ---

extern "C" void app_main(void)
{
    // Let the board finish its own boot before running tests
    vTaskDelay(pdMS_TO_TICKS(2000));

    UNITY_BEGIN();
    RUN_TEST(test_initial_state_is_idle);
    RUN_TEST(test_start_waiting_sets_booting_state_and_creates_task);
    RUN_TEST(test_start_waiting_is_idempotent_when_already_running);
    RUN_TEST(test_notify_success_sets_stop_flag);
    RUN_TEST(test_notify_success_on_idle_indicator_is_safe_noop);
    RUN_TEST(test_notify_failure_on_idle_starts_task_in_failed_state);
    RUN_TEST(test_notify_failure_during_booting_switches_to_failed_state);
    RUN_TEST(test_notify_failure_on_already_failed_indicator_is_idempotent);
    RUN_TEST(test_task_self_clears_after_success_is_signalled);
    UNITY_END();
}
