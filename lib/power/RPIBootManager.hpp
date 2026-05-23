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
        EventGroupHandle_t mp_rpiBootEventGroup = nullptr;
        static constexpr int ms_rpiHeartbeatBit = (1 << 0);
        static constexpr int ms_rpiShutdownBit = (1 << 1);
        static constexpr const char *k_logTag = "Rpi_Boot_Manager";
    };
}