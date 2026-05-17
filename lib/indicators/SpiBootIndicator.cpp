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

        while (!self->mStop)
        {
            const uint32_t halfPeriod =
                (self->mState == State::Failed) ? kFailedHalfPeriodMs : kBootHalfPeriodMs;

            getSpiLedDriver().setAllLeds(ledsOn);
            ledsOn = !ledsOn;

            vTaskDelay(pdMS_TO_TICKS(halfPeriod));
        }

        // Clear all LEDs and clean up
        getSpiLedDriver().setAllLeds(false);
        self->mTask  = nullptr;
        self->mState = State::Idle;
        vTaskDelete(nullptr);
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    void SpiBootIndicator::startTask()
    {
        mStop = false;
        xTaskCreate(
            flashTask,
            "spi_boot_ind",
            2048,
            this,
            tskIDLE_PRIORITY + 1,
            &mTask);
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
        ESP_LOGI(mspTag, "Starting boot-wait flash (500 ms)");
        mState = State::Booting;
        startTask();
    }

    void SpiBootIndicator::notifySuccess()
    {
        if (mTask == nullptr)
        {
            return; // nothing running, nothing to do
        }
        ESP_LOGI(mspTag, "RPi boot confirmed — stopping boot indicator");
        mStop = true;
        // The task will clear the LEDs and delete itself on its next wake
    }

    void SpiBootIndicator::notifyFailure()
    {
        if (mTask != nullptr)
        {
            // Task already running (timeout case) — switch to the fast failed pattern
            ESP_LOGW(mspTag, "Boot failed — switching to fast-fail flash (150 ms)");
            mState = State::Failed;
        }
        else
        {
            // Task not yet running (firmware init failure case) — start in failed state
            ESP_LOGW(mspTag, "Firmware init failed — starting fast-fail flash (150 ms)");
            mState = State::Failed;
            startTask();
        }
    }
}
