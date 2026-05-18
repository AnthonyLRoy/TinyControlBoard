#include "SpiBootIndicator.hpp"

#include "indicators/ledManager.hpp"
#include "esp_log.h"

namespace indicators
{
    // -------------------------------------------------------------------------
    // Internal FreeRTOS task
    // -------------------------------------------------------------------------

    void SpiBootIndicator::flashTask(void *arg)
    {
        auto *self = static_cast<SpiBootIndicator *>(arg);
        bool ledsOn = false;

        while (!self->mStop.load(std::memory_order_acquire))
        {
            const uint32_t halfPeriod =
                (self->mState.load(std::memory_order_relaxed) == State::Failed) ? kFailedHalfPeriodMs : kBootHalfPeriodMs;

            getSpiLedDriver().setAllLeds(ledsOn);
            ledsOn = !ledsOn;

            vTaskDelay(pdMS_TO_TICKS(halfPeriod));
        }

        // Clear all LEDs and clean up
        getSpiLedDriver().setAllLeds(false);
        self->mTask  = nullptr;
        self->mState.store(State::Idle, std::memory_order_release);
        vTaskDelete(nullptr);
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    void SpiBootIndicator::startTask()
    {
        mStop.store(false, std::memory_order_release);
        if (xTaskCreate(
            flashTask,
            "spi_boot_ind",
            4096,
            this,
            tskIDLE_PRIORITY + 1,
            &mTask) != pdPASS)
        {
            mTask = nullptr;
            mStop.store(true, std::memory_order_release);
            mState.store(State::Idle, std::memory_order_release);
            ESP_LOGE(kLogTag, "Failed to create SPI boot indicator task");
        }
    }

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------

    void SpiBootIndicator::startWaiting()
    {
        if (mTask != nullptr)
        {
            return; // already running
        }
        ESP_LOGI(kLogTag, "Starting boot-wait flash (500 ms)");
        mState.store(State::Booting, std::memory_order_release);
        startTask();
    }

    void SpiBootIndicator::notifySuccess()
    {
        if (mTask == nullptr)
        {
            return; // nothing running, nothing to do
        }
        ESP_LOGI(kLogTag, "RPi boot confirmed — stopping boot indicator");
        mStop.store(true, std::memory_order_release);
        // The task will clear the LEDs and delete itself on its next wake
    }

    void SpiBootIndicator::notifyFailure()
    {
        if (mTask != nullptr)
        {
            // Task already running (timeout case) — switch to the fast failed pattern
            ESP_LOGW(kLogTag, "Boot failed — switching to fast-fail flash (150 ms)");
            mState.store(State::Failed, std::memory_order_release);
        }
        else
        {
            // Task not yet running (firmware init failure case) — start in failed state
            ESP_LOGW(kLogTag, "Firmware init failed — starting fast-fail flash (150 ms)");
            mState.store(State::Failed, std::memory_order_release);
            startTask();
        }
    }
}
