#include "heartbeatMonitor.hpp"

#include <esp_log.h>
#include <esp_timer.h>

namespace
{
    constexpr uint32_t k_checkIntervalMs = 100;
    constexpr uint32_t k_taskStackSize = 4096;
    constexpr uint32_t k_taskPriority = 5;
    constexpr const char *k_logTag = "Serial          ";
}

namespace transport::uart
{
    // Stops the watchdog worker when the monitor is destroyed.
    HeartbeatMonitor::~HeartbeatMonitor()
    {
        stop();
    }

    // Configures the watchdog and starts the task that checks for missing UART traffic.
    void HeartbeatMonitor::start(uint32_t timeoutMs, std::function<void()> onTimeout)
    {
        m_watchdog.setTimeoutMs(timeoutMs);
        m_onTimeout = std::move(onTimeout);
        m_watchdog.notifyRx(esp_timer_get_time());

        if (mp_taskHandle.load(std::memory_order_acquire) != nullptr)
        {
            return;
        }

        m_stopTask.store(false, std::memory_order_release);
        TaskHandle_t handle = nullptr;
        if (xTaskCreate(taskTrampoline, "heartbeat_task", k_taskStackSize, this, k_taskPriority, &handle) != pdPASS)
        {
            ESP_LOGE(k_logTag, "Failed to create heartbeat monitor task");
            return;
        }
        mp_taskHandle.store(handle, std::memory_order_release);
    }

    // Requests the watchdog task to stop and waits briefly before forcing deletion if needed.
    void HeartbeatMonitor::stop()
    {
        TaskHandle_t handle = mp_taskHandle.load(std::memory_order_acquire);
        if (handle == nullptr)
        {
            return;
        }

        m_stopTask.store(true, std::memory_order_release);
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            for (int i = 0; mp_taskHandle.load(std::memory_order_acquire) && i < 50; ++i)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        if (mp_taskHandle.load(std::memory_order_acquire))
        {
            ESP_LOGW(k_logTag, "Heartbeat task did not stop in time; forcing delete");
            vTaskDelete(handle);
            mp_taskHandle.store(nullptr, std::memory_order_release);
        }
    }

    // FreeRTOS entry point that forwards task startup to the monitor instance.
    void HeartbeatMonitor::taskTrampoline(void *p_arg)
    {
        static_cast<HeartbeatMonitor *>(p_arg)->run();
    }

    // Periodically checks the watchdog and invokes the timeout callback when UART activity expires.
    void HeartbeatMonitor::run()
    {
        const TickType_t delay = pdMS_TO_TICKS(k_checkIntervalMs);

        while (!m_stopTask.load(std::memory_order_acquire))
        {
            vTaskDelay(delay);

            if (m_stopTask.load(std::memory_order_acquire))
            {
                break;
            }

            if (m_watchdog.checkAndConsumeTimeout(esp_timer_get_time()) && m_onTimeout)
            {
                m_onTimeout();
            }
        }

        mp_taskHandle.store(nullptr, std::memory_order_release);
        vTaskDelete(nullptr);
    }
} // namespace transport::uart
