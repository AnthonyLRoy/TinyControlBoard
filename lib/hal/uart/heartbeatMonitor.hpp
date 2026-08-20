#pragma once

#include "heartbeatWatchdog.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>
#include <functional>

namespace transport::uart
{
    // Owns the FreeRTOS task lifecycle around a HeartbeatWatchdog and fires
    // onTimeout() from the monitor task when the watchdog trips.
    class HeartbeatMonitor
    {
    public:
        ~HeartbeatMonitor();

        void start(uint32_t timeoutMs, std::function<void()> onTimeout);
        void stop();

        void notifyRx(uint64_t nowUs) { m_watchdog.notifyRx(nowUs); }
        uint64_t getLastRxTimeUs() const { return m_watchdog.getLastRxTimeUs(); }

    private:
        static void taskTrampoline(void *p_arg);
        void run();

        HeartbeatWatchdog m_watchdog;
        std::function<void()> m_onTimeout;
        // Atomic: written by run() on self-delete, read from stop() on the caller's task.
        std::atomic<TaskHandle_t> mp_taskHandle{nullptr};
        std::atomic<bool> m_stopTask{false};
    };
} // namespace transport::uart
