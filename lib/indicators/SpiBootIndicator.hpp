#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include <cstdint>

namespace indicators
{
    /// Flashes all 16 SPI button LEDs to indicate the RPi boot state.
    ///
    /// - Booting  (startWaiting):   all LEDs on/off at 500 ms half-period — progress indicator
    /// - Failed   (notifyFailure):  all LEDs on/off at 150 ms half-period — error indicator
    /// - Stopped  (notifySuccess):  all LEDs off, task deleted
    class SpiBootIndicator
    {
    public:
        /// Start flashing in "waiting for RPi boot" mode (500 ms half-period).
        /// No-op if already running.
        // Starts the boot-wait flash pattern while the Raspberry Pi is still starting up.
        void startWaiting();

        /// RPi heartbeat received — stop flashing and clear all LEDs.
        // Stops the boot flash and clears all SPI LEDs once the Pi is healthy.
        void notifySuccess();

        /// Boot failed (timeout or firmware init error).
        /// If the task is already running (timeout case), switches to the faster 150 ms pattern.
        /// If the task is not yet running (firmware init failure), starts it in the failed state.
        // Switches the indicator to the failure flash pattern when the boot sequence is not successful.
        void notifyFailure();

    private:
        enum class State : uint8_t { Idle, Booting, Failed };

        static constexpr uint32_t k_bootHalfPeriodMs   = 500;
        static constexpr uint32_t k_failedHalfPeriodMs = 150;
        static constexpr const char *k_logTag = "SpiBootIndicator";

        std::atomic<State> m_state{State::Idle};
        std::atomic<bool> m_stop{false};
        TaskHandle_t   m_task    = nullptr;

        void startTask();
        static void flashTask(void *arg);
    };
}
