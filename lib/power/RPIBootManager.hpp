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
        RpiBootManager();
        ~RpiBootManager();

        void handleHeartbeatReceived();
        void handleHeartbeatTimeout();
        bool waitForRpiToBoot(uint32_t timeoutMs = 60000);
        bool waitForRpiShutdown(uint32_t timeoutMs = 60000);

    private:
        EventGroupHandle_t mpRpiBootEventGroup = nullptr;
        static constexpr int msRpiHeartbeatBit = (1 << 0);
        static constexpr int msRpiShutdownBit = (1 << 1);
        static constexpr const char *mspTag = "RPI_Boot_Manager";
    };
}