#pragma once

#include <atomic>
#include <cstdint>

namespace transport::uart
{
    // Pure timeout-decision logic for heartbeat monitoring. No ESP-IDF/FreeRTOS
    // dependency so it can be exercised directly from host_tests.
    class HeartbeatWatchdog
    {
    public:
        // Creates a watchdog with an optional timeout duration in milliseconds.
        explicit HeartbeatWatchdog(uint32_t timeoutMs = 0) : m_timeoutMs(timeoutMs) {}

        // Sets the elapsed-time threshold that triggers a heartbeat timeout.
        void setTimeoutMs(uint32_t timeoutMs) { m_timeoutMs = timeoutMs; }
        // Returns the configured heartbeat timeout in milliseconds.
        uint32_t getTimeoutMs() const { return m_timeoutMs; }

        // Records the timestamp of the most recent received UART data.
        void notifyRx(uint64_t nowUs) { m_lastRxTimeUs.store(nowUs, std::memory_order_relaxed); }

        // Returns the timestamp of the most recently recorded UART data.
        uint64_t getLastRxTimeUs() const { return m_lastRxTimeUs.load(std::memory_order_relaxed); }

        // Returns true once when more than timeoutMs has elapsed since the last recorded RX
        // (or since the last timeout fired). A zero last-RX time means "never armed" and never times out.
        bool checkAndConsumeTimeout(uint64_t nowUs)
        {
            const uint64_t last = m_lastRxTimeUs.load(std::memory_order_relaxed);
            if (last == 0)
            {
                return false;
            }

            const uint64_t diffMs = (nowUs - last) / 1000;
            if (diffMs > m_timeoutMs)
            {
                m_lastRxTimeUs.store(nowUs, std::memory_order_relaxed);
                return true;
            }
            return false;
        }

    private:
        uint32_t m_timeoutMs;
        std::atomic<uint64_t> m_lastRxTimeUs{0};
    };
} // namespace transport::uart
