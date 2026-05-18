#include "power/RPIBootManager.hpp"

#include "board/boardConfig.hpp"
#include <inttypes.h>

namespace controlSystem
{
    RpiBootManager::RpiBootManager()
    {
        mpRpiBootEventGroup = xEventGroupCreate();
        if (mpRpiBootEventGroup == nullptr) {
            ESP_LOGE(kLogTag, "Failed to create RPi boot event group");
        }
    }

    RpiBootManager::~RpiBootManager()
    {
        if (mpRpiBootEventGroup != nullptr) {
            vEventGroupDelete(mpRpiBootEventGroup);
            mpRpiBootEventGroup = nullptr;
        }
    }

    void RpiBootManager::handleHeartbeatReceived()
    {
        if (mpRpiBootEventGroup != nullptr) {
            xEventGroupSetBits(mpRpiBootEventGroup, msRpiHeartbeatBit);
            ESP_LOGI(kLogTag, "Heartbeat received from RPi; boot complete or system still active");
        }
    }

    void RpiBootManager::handleHeartbeatTimeout()
    {
        if (mpRpiBootEventGroup != nullptr) {
            xEventGroupSetBits(mpRpiBootEventGroup, msRpiShutdownBit);
            ESP_LOGW(kLogTag, "Heartbeat timeout detected; RPi shut down or unavailable");
        }
    }

    bool RpiBootManager::waitForRpiToBoot(uint32_t timeoutMs)
    {
        if constexpr (board::debug::kSimulateRpiBoot)
        {
            ESP_LOGW(kLogTag, "Debug mode enabled: skipping RPi heartbeat wait (kSimulateRpiBoot=true)");
            return true;
        }

        if (mpRpiBootEventGroup == nullptr) {
            ESP_LOGE(kLogTag, "Event group not initialized");
            return false;
        }

        // Start each wait from a clean state so stale events from previous cycles
        // cannot satisfy the current lifecycle transition.
        const EventBits_t clearMask = static_cast<EventBits_t>(msRpiHeartbeatBit | msRpiShutdownBit);
        xEventGroupClearBits(mpRpiBootEventGroup, clearMask);

        ESP_LOGI(kLogTag, "Waiting for RPi heartbeat (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            mpRpiBootEventGroup,
            msRpiHeartbeatBit,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & msRpiHeartbeatBit) {
            ESP_LOGI(kLogTag, "RPi heartbeat detected; boot successful");
            return true;
        }

        ESP_LOGW(kLogTag, "Timed out waiting for RPi heartbeat after %" PRIu32 " ms", timeoutMs);
        return false;
    }

    bool RpiBootManager::waitForRpiShutdown(uint32_t timeoutMs)
    {
        if (mpRpiBootEventGroup == nullptr) {
            ESP_LOGE(kLogTag, "Event group not initialized");
            return false;
        }

        const EventBits_t clearMask = static_cast<EventBits_t>(msRpiHeartbeatBit | msRpiShutdownBit);
        xEventGroupClearBits(mpRpiBootEventGroup, clearMask);

        ESP_LOGI(kLogTag, "Waiting for RPi shutdown confirmation (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            mpRpiBootEventGroup,
            msRpiShutdownBit,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & msRpiShutdownBit) {
            ESP_LOGI(kLogTag, "RPi shutdown confirmed by heartbeat timeout");
            return true;
        }

        ESP_LOGW(kLogTag, "Timed out waiting for RPi shutdown after %" PRIu32 " ms", timeoutMs);
        return false;
    }
}