#include "RPIBootManager.hpp"
#include <inttypes.h>

namespace controlSystem
{
    RPIBootManager::RPIBootManager()
    {
        rpi_boot_event_group = xEventGroupCreate();
        if (rpi_boot_event_group == nullptr) {
            ESP_LOGE(TAG, "Failed to create RPI boot event group");
        }
    }

    RPIBootManager::~RPIBootManager()
    {
        if (rpi_boot_event_group != nullptr) {
            vEventGroupDelete(rpi_boot_event_group);
            rpi_boot_event_group = nullptr;
        }
    }

    void RPIBootManager::onHeartbeatReceived()
    {
        if (rpi_boot_event_group != nullptr) {
            xEventGroupSetBits(rpi_boot_event_group, RPI_HEARTBEAT_BIT);
            //ESP_LOGI(TAG, "Heartbeat received from RPI - boot complete or still active");
        }
    }

    void RPIBootManager::onHeartbeatTimeout()
    {
        if (rpi_boot_event_group != nullptr) {
            xEventGroupSetBits(rpi_boot_event_group, RPI_SHUTDOWN_BIT);
            ESP_LOGI(TAG, "Heartbeat timeout detected - RPI has shut down, or not avaialble ");
        }
    }

    bool RPIBootManager::WaitForRpiToBoot(uint32_t timeoutMs)
    {
        if (rpi_boot_event_group == nullptr) {
            ESP_LOGE(TAG, "Event group not initialized");
            return false;
        }

        ESP_LOGI(TAG, "Waiting for RPI heartbeat (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            rpi_boot_event_group,
            RPI_HEARTBEAT_BIT,
            pdTRUE,  // Clear bits on exit
            pdFALSE, // Don't wait for all bits
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & RPI_HEARTBEAT_BIT) {
            ESP_LOGI(TAG, "RPI heartbeat detected - boot successful");
            return true;
        } else {
            ESP_LOGW(TAG, "Timeout waiting for RPI heartbeat after %" PRIu32 " ms", timeoutMs);
            return false;
        }
    }

    bool RPIBootManager::waitForPiShutdown(uint32_t timeoutMs)
    {
        if (rpi_boot_event_group == nullptr) {
            ESP_LOGE(TAG, "Event group not initialized");
            return false;
        }

        ESP_LOGI(TAG, "Waiting for RPI shutdown confirmation (timeout: %" PRIu32 " ms)...", timeoutMs);

        EventBits_t bits = xEventGroupWaitBits(
            rpi_boot_event_group,
            RPI_SHUTDOWN_BIT,
            pdTRUE,  // Clear bits on exit
            pdFALSE, // Don't wait for all bits
            pdMS_TO_TICKS(timeoutMs)
        );

        if (bits & RPI_SHUTDOWN_BIT) {
            ESP_LOGI(TAG, "RPI shutdown confirmed - heartbeat timeout detected");
            return true;
        } else {
            ESP_LOGW(TAG, "Timeout waiting for RPI shutdown after %" PRIu32 " ms", timeoutMs);
            return false;
        }
    }
}
