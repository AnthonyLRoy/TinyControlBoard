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
    HeartbeatMonitor::~HeartbeatMonitor()
    {
        stop();
    }

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

    void HeartbeatMonitor::taskTrampoline(void *p_arg)
    {
        static_cast<HeartbeatMonitor *>(p_arg)->run();
    }

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
