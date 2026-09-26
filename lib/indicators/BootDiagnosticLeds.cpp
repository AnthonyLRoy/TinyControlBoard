#include "indicators/BootDiagnosticLeds.hpp"

#include "indicators/ledManager.hpp"
#include "esp_log.h"

namespace indicators
{
    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    // Converts a boot stage to the two-bit mask for its diagnostic LED pair.
    uint16_t BootDiagnosticLeds::stageMaskFor(BootStage stage)
    {
        const auto idx = static_cast<uint8_t>(stage);
        return static_cast<uint16_t>((1u << k_stageBits[idx][0]) | (1u << k_stageBits[idx][1]));
    }

    // Starts the failure-flash task for the specified LED mask unless failure indication is already active.
    void BootDiagnosticLeds::startFlashTask(uint16_t mask)
    {
        if (m_failureActive.exchange(true, std::memory_order_acq_rel))
        {
            return; // already indicating a failure
        }

        m_flashMask.store(mask, std::memory_order_release);

        if (xTaskCreate(flashTask, "boot_diag_leds", 2048u, this,
                        tskIDLE_PRIORITY + 1, &m_task) != pdPASS)
        {
            m_task = nullptr;
            ESP_LOGE(k_logTag, "Failed to create boot diagnostic flash task");
        }
    }

    // -------------------------------------------------------------------------
    // Turns off non-failing diagnostic LEDs and flashes the selected mask until the task is stopped.
    // -------------------------------------------------------------------------

    // FreeRTOS worker that repeatedly applies the selected failure pattern to the SPI LEDs.
    void BootDiagnosticLeds::flashTask(void *arg)
    {
        auto *self = static_cast<BootDiagnosticLeds *>(arg);
        const uint16_t mask = self->m_flashMask.load(std::memory_order_acquire);

        auto &driver = getSpiLedDriver();

        for (uint8_t i = 0u; i < 16u; ++i)
        {
            if (k_allDiagnosticBits & static_cast<uint16_t>(1u << i))
            {
                driver.setLed(i, false);
            }
        }

        bool ledsOn = false;
        while (true)
        {
            for (uint8_t i = 0u; i < 16u; ++i)
            {
                if (mask & static_cast<uint16_t>(1u << i))
                {
                    driver.setLed(i, ledsOn);
                }
            }
            ledsOn = !ledsOn;
            vTaskDelay(pdMS_TO_TICKS(k_flashHalfPeriodMs));
        }
    }


    // Stops any previous failure pattern and turns on all diagnostic LEDs for a new boot attempt.
    void BootDiagnosticLeds::begin()
    {
        // Stop any failure flash task from a previous (failed) attempt.
        if (m_failureActive.load(std::memory_order_acquire))
        {
            if (m_task != nullptr)
            {
                vTaskDelete(m_task);
                m_task = nullptr;
            }
            m_failureActive.store(false, std::memory_order_release);
        }

        ESP_LOGI(k_logTag, "Boot diagnostic LEDs: initial state (8 LEDs on)");
        auto &driver = getSpiLedDriver();
        for (uint8_t i = 0u; i < 16u; ++i)
        {
            if (k_allDiagnosticBits & static_cast<uint16_t>(1u << i))
            {
                driver.setLed(i, true);
            }
        }
    }

    // Marks a boot stage as successful by switching off its associated LED pair.
    void BootDiagnosticLeds::stageSuccess(BootStage stage)
    {
        const auto idx = static_cast<uint8_t>(stage);
        ESP_LOGI(k_logTag, "Boot stage %u success: LEDs extinguished", idx);
        auto &driver = getSpiLedDriver();
        driver.setLed(k_stageBits[idx][0], false);
        driver.setLed(k_stageBits[idx][1], false);
    }

    // Starts flashing only the LED pair associated with the failed boot stage.
    void BootDiagnosticLeds::stageFailure(BootStage stage)
    {
        const auto idx = static_cast<uint8_t>(stage);
        ESP_LOGW(k_logTag, "Boot stage %u failed: flashing LED pair at 3 Hz", idx);
        startFlashTask(stageMaskFor(stage));
    }

    // Starts flashing all diagnostic LEDs to indicate firmware initialization failure.
    void BootDiagnosticLeds::firmwareInitFailed()
    {
        ESP_LOGW(k_logTag, "Firmware init failed: flashing all diagnostic LEDs at 3 Hz");
        startFlashTask(k_allDiagnosticBits);
    }

} // namespace indicators
