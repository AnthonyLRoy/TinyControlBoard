#include "rpiBootManager.hpp"
#include <inttypes.h>

namespace controlSystem
{
    RpiBootManager::RpiBootManager()
    {
        mpRpiBootEventGroup = xEventGroupCreate();
        if (mpRpiBootEventGroup == nullptr) {
            ESP_LOGE(mspTag, "Failed to create RPI boot event group");
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
            //ESP_LOGI(TAG, "Heartbeat received from RPI - boot complete or system still active");
        }
    }

    void RpiBootManager::handleHeartbeatTimeout()
    {
        if (mpRpiBootEventGroup != nullptr) {
            xEventGroupSetBits(mpRpiBootEventGroup, msRpiShutdownBit);
            ESP_LOGI(mspTag, "Heartbeat timeout detected - RPI has shut down, or not available ");
        }
    }

    bool RpiBootManager::waitForRpiToBoot(uint32_t timeoutMs)
    {
        if (mpRpiBootEventGroup == nullptr) {
            ESP_LOGE(mspTag, "Event group not initialized");
            return false;
        }

        ESP_LOGI(mspTag, "Waiting for RPI heartbeat (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            mpRpiBootEventGroup,
            msRpiHeartbeatBit,
            pdTRUE,  // Clear bits on exit
            pdFALSE, // Don't wait for all bits
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & msRpiHeartbeatBit) {
            ESP_LOGI(mspTag, "RPI heartbeat detected - boot successful");
            return true;
        } else {
            ESP_LOGW(mspTag, "Timeout waiting for RPI heartbeat after %" PRIu32 " ms", timeoutMs);
            return false;
        }
    }

    bool RpiBootManager::waitForRpiShutdown(uint32_t timeoutMs)
    {
        if (mpRpiBootEventGroup == nullptr) {
            ESP_LOGE(mspTag, "Event group not initialized");
            return false;
        }

        ESP_LOGI(mspTag, "Waiting for RPI shutdown confirmation (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            mpRpiBootEventGroup,
            msRpiShutdownBit,
            pdTRUE,  // Clear bits on exit
            pdFALSE, // Don't wait for all bits
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & msRpiShutdownBit) {
            ESP_LOGI(mspTag, "RPI shutdown confirmed - heartbeat timeout detected");
            return true;
        } else {
            ESP_LOGW(mspTag, "Timeout waiting for RPI shutdown after %" PRIu32 " ms", timeoutMs);
            return false;
        }
    }
}
