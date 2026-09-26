#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <cstdint>
#include "esp_log.h"

namespace controlSystem
{
    class RpiBootManager
    {
    public:
        // Creates the boot monitor that tracks Raspberry Pi startup and shutdown acknowledgement.
        RpiBootManager();
        // Releases any boot-status event resources owned by the manager.
        ~RpiBootManager();

        // Records that a valid Raspberry Pi heartbeat has been received.
        void handleHeartbeatReceived();
        // Marks the Raspberry Pi as timed out and signals the boot watchdog path.
        void handleHeartbeatTimeout();
        // Waits until the Raspberry Pi heartbeat confirms the board has finished booting.
        bool waitForRpiToBoot(uint32_t timeoutMs = 60000);
        // Waits until the Raspberry Pi signals a clean shutdown before continuing power-down.
        bool waitForRpiShutdown(uint32_t timeoutMs = 60000);

    private:
        EventGroupHandle_t mp_rpiBootEventGroup = nullptr;
        static constexpr int ms_rpiHeartbeatBit = (1 << 0);
        static constexpr int ms_rpiShutdownBit = (1 << 1);
        static constexpr const char *k_logTag = "Rpi_Boot_Manager";
    };
}