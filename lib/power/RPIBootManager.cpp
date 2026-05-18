#include "power/RPIBootManager.hpp"

#include "board/boardConfig.hpp"
#include <inttypes.h>

namespace controlSystem
{
    RpiBootManager::RpiBootManager()
    {
        mp_rpiBootEventGroup = xEventGroupCreate();
        if (mp_rpiBootEventGroup == nullptr) {
            ESP_LOGE(k_logTag, "Failed to create RPi boot event group");
        }
    }

    RpiBootManager::~RpiBootManager()
    {
        if (mp_rpiBootEventGroup != nullptr) {
            vEventGroupDelete(mp_rpiBootEventGroup);
            mp_rpiBootEventGroup = nullptr;
        }
    }

    void RpiBootManager::handleHeartbeatReceived()
    {
        if (mp_rpiBootEventGroup != nullptr) {
            xEventGroupSetBits(mp_rpiBootEventGroup, ms_rpiHeartbeatBit);
            ESP_LOGI(k_logTag, "Heartbeat received from RPi; boot complete or system still active");
        }
    }

    void RpiBootManager::handleHeartbeatTimeout()
    {
        if (mp_rpiBootEventGroup != nullptr) {
            xEventGroupSetBits(mp_rpiBootEventGroup, ms_rpiShutdownBit);
            ESP_LOGW(k_logTag, "Heartbeat timeout detected; RPi shut down or unavailable");
        }
    }

    bool RpiBootManager::waitForRpiToBoot(uint32_t timeoutMs)
    {
        if constexpr (board::debug::k_simulateRpiBoot)
        {
            ESP_LOGW(k_logTag, "Debug mode enabled: skipping RPi heartbeat wait (k_simulateRpiBoot=true)");
            return true;
        }

        if (mp_rpiBootEventGroup == nullptr) {
            ESP_LOGE(k_logTag, "Event group not initialized");
            return false;
        }

        // Start each wait from a clean state so stale events from previous cycles
        // cannot satisfy the current lifecycle transition.
        const EventBits_t clearMask = static_cast<EventBits_t>(ms_rpiHeartbeatBit | ms_rpiShutdownBit);
        xEventGroupClearBits(mp_rpiBootEventGroup, clearMask);

        ESP_LOGI(k_logTag, "Waiting for RPi heartbeat (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            mp_rpiBootEventGroup,
            ms_rpiHeartbeatBit,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & ms_rpiHeartbeatBit) {
            ESP_LOGI(k_logTag, "RPi heartbeat detected; boot successful");
            return true;
        }

        ESP_LOGW(k_logTag, "Timed out waiting for RPi heartbeat after %" PRIu32 " ms", timeoutMs);
        return false;
    }

    bool RpiBootManager::waitForRpiShutdown(uint32_t timeoutMs)
    {
        if (mp_rpiBootEventGroup == nullptr) {
            ESP_LOGE(k_logTag, "Event group not initialized");
            return false;
        }

        const EventBits_t clearMask = static_cast<EventBits_t>(ms_rpiHeartbeatBit | ms_rpiShutdownBit);
        xEventGroupClearBits(mp_rpiBootEventGroup, clearMask);

        ESP_LOGI(k_logTag, "Waiting for RPi shutdown confirmation (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            mp_rpiBootEventGroup,
            ms_rpiShutdownBit,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & ms_rpiShutdownBit) {
            ESP_LOGI(k_logTag, "RPi shutdown confirmed by heartbeat timeout");
            return true;
        }

        ESP_LOGW(k_logTag, "Timed out waiting for RPi shutdown after %" PRIu32 " ms", timeoutMs);
        return false;
    }
}