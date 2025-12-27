#include "RelayController.hpp"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace controlSystem
{
    RelayController::RelayController(serialBus::Serial &serialRef, relays::StandardRelay &relaysRef)
        : serial(serialRef), relays(relaysRef)
    {
    }

    void RelayController::setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs)
    {
        ESP_LOGI(TAG, "Setting relay %d to %s with delay %lu ms", pin, state ? "ON" : "OFF", delayMs);
        relays.setRelayState(pin, state);
        if (delayMs > 0) {
            vTaskDelay(pdMS_TO_TICKS(delayMs));
        }
    }

    bool RelayController::HandleToggleDac(bool state)
    {
        relays.setRelayState(PIN_RELAY_DAC_POWER, state);
        ESP_LOGI(TAG, "DAC relay set to %s", state ? "ON" : "OFF");
        return true;
    }

    bool RelayController::ShutDownRPI(bool wait)
    {
        relays.setRelayState(PIN_RELAY_RPI_POWER, false);
        ESP_LOGI(TAG, "RPI relay disabled");
        return true;
    }

    bool RelayController::ShutDownScreen(bool wait)
    {
        relays.setRelayState(PIN_RELAY_SCREEN_POWER, false);
        ESP_LOGI(TAG, "Screen relay disabled");
        return true;
    }
}
