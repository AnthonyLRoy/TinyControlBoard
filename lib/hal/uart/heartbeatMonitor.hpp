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
        // Stops any running timeout task and releases monitor resources.
        ~HeartbeatMonitor();

        // Starts the watchdog task that fires when UART activity stops for too long.
        void start(uint32_t timeoutMs, std::function<void()> onTimeout);
        // Stops the watchdog task and resets the monitor state.
        void stop();

        // Records the current timestamp when valid RX traffic has been seen.
        void notifyRx(uint64_t nowUs) { m_watchdog.notifyRx(nowUs); }
        // Returns the timestamp of the most recent RX activity.
        uint64_t getLastRxTimeUs() const { return m_watchdog.getLastRxTimeUs(); }

    private:
        // FreeRTOS task trampoline used to invoke the instance watchdog loop.
        static void taskTrampoline(void *p_arg);
        // Runs the timeout monitoring loop until the watchdog is stopped.
        void run();

        HeartbeatWatchdog m_watchdog;
        std::function<void()> m_onTimeout;
        // Atomic: written by run() on self-delete, read from stop() on the caller's task.
        std::atomic<TaskHandle_t> mp_taskHandle{nullptr};
        std::atomic<bool> m_stopTask{false};
    };
} // namespace transport::uart
