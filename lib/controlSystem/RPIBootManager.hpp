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
    class RPIBootManager
    {
    public:
        RPIBootManager();
        ~RPIBootManager();

        // Called when heartbeat is received from RPI (boot detection)
        void onHeartbeatReceived();

        // Called when heartbeat timeout occurs (RPI offline detection)
        void onHeartbeatTimeout();

        // Wait for RPI to boot (heartbeat reception)
        bool WaitForRpiToBoot(uint32_t timeoutMs = 60000);

        // Wait for RPI to shutdown (heartbeat timeout)
        bool waitForPiShutdown(uint32_t timeoutMs = 60000);

    private:
        EventGroupHandle_t rpi_boot_event_group = nullptr;
        static constexpr int RPI_HEARTBEAT_BIT = (1 << 0);  // Heartbeat received
        static constexpr int RPI_SHUTDOWN_BIT = (1 << 1);   // Heartbeat timeout
        static constexpr const char *TAG = "RPIBootManager";
    };
}
