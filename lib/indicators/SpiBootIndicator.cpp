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

        while (!self->m_stop.load(std::memory_order_acquire))
        {
            const uint32_t halfPeriod =
                (self->m_state.load(std::memory_order_relaxed) == State::Failed) ? k_failedHalfPeriodMs : k_bootHalfPeriodMs;

            getSpiLedDriver().setAllLeds(ledsOn);
            ledsOn = !ledsOn;

            vTaskDelay(pdMS_TO_TICKS(halfPeriod));
        }

        // Clear all LEDs and clean up
        getSpiLedDriver().setAllLeds(false);
        self->m_task  = nullptr;
        self->m_state.store(State::Idle, std::memory_order_release);
        vTaskDelete(nullptr);
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    void SpiBootIndicator::startTask()
    {
        m_stop.store(false, std::memory_order_release);
        if (xTaskCreate(
            flashTask,
            "spi_boot_ind",
            4096,
            this,
            tskIDLE_PRIORITY + 1,
            &m_task) != pdPASS)
        {
            m_task = nullptr;
            m_stop.store(true, std::memory_order_release);
            m_state.store(State::Idle, std::memory_order_release);
            ESP_LOGE(k_logTag, "Failed to create SPI boot indicator task");
        }
    }

    // -------------------------------------------------------------------------
    // Public API
  

    void SpiBootIndicator::startWaiting()
    {
        if (m_task != nullptr)
        {
            return; // already running
        }
        ESP_LOGI(k_logTag, "Starting boot-wait flash (500 ms)");
        m_state.store(State::Booting, std::memory_order_release);
        startTask();
    }

    void SpiBootIndicator::notifySuccess()
    {
        if (m_task == nullptr)
        {
            return; // nothing running, nothing to do
        }
        ESP_LOGI(k_logTag, "RPi boot confirmed — stopping boot indicator");
        m_stop.store(true, std::memory_order_release);
        // The task will clear the LEDs and delete itself on its next wake
    }

    void SpiBootIndicator::notifyFailure()
    {
        if (m_task != nullptr)
        {
            // Task already running (timeout case) — switch to the fast failed pattern
            ESP_LOGW(k_logTag, "Boot failed — switching to fast-fail flash (150 ms)");
            m_state.store(State::Failed, std::memory_order_release);
        }
        else
        {
            // Task not yet running (firmware init failure case) — start in failed state
            ESP_LOGW(k_logTag, "Firmware init failed — starting fast-fail flash (150 ms)");
            m_state.store(State::Failed, std::memory_order_release);
            startTask();
        }
    }
}
