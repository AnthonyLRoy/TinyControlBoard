#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <cstdint>
#include "esp_log.h"

namespace controlSystem
{
    /**
     * Manages RPI boot and shutdown synchronization via heartbeat monitoring.
     * Handles the event-based synchronization for RPI lifecycle events.
     */
    class RpiBootManager
    {
    public:
        RpiBootManager();
        ~RpiBootManager();

        // Called when heartbeat is received from RPI (boot detection)
        void handleHeartbeatReceived();

        // Called when heartbeat timeout occurs (RPI offline detection)
        void handleHeartbeatTimeout();

        // Wait for RPI to boot (heartbeat reception)
        bool waitForRpiToBoot(uint32_t timeoutMs = 60000);

        // Wait for RPI to shutdown (heartbeat timeout)
        bool waitForRpiShutdown(uint32_t timeoutMs = 60000);

    private:
        EventGroupHandle_t mpRpiBootEventGroup = nullptr;
        static constexpr int msRpiHeartbeatBit = (1 << 0);  // Heartbeat received
        static constexpr int msRpiShutdownBit = (1 << 1);   // Heartbeat timeout
        static constexpr const char *mspTag = "RpiBootManager";
    };
}
