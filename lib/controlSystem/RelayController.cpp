#include "relayController.hpp"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace controlSystem
{
    RelayController::RelayController(serialBus::Serial &rSerial, relays::StandardRelay &rRelays)
        : mrSerial(rSerial), mrRelays(rRelays)
    {
    }

    void RelayController::setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs)
    {
        ESP_LOGI(mspTag, "Setting relay %d to %s with delay %lu ms", pin, state ? "ON" : "OFF", delayMs);
        mrRelays.setRelayState(pin, state);
        if (delayMs > 0) {
            vTaskDelay(pdMS_TO_TICKS(delayMs));
        }
    }

    bool RelayController::handleToggleDac(bool state)
    {
        mrRelays.setRelayState(PIN_RELAY_DAC_POWER, state);
        ESP_LOGI(mspTag, "DAC relay set to %s", state ? "ON" : "OFF");
        return true;
    }

    bool RelayController::shutdownRpi(bool wait)
    {
        mrRelays.setRelayState(PIN_RELAY_RPI_POWER, false);
        ESP_LOGI(mspTag, "RPI relay disabled");
        return true;
    }

    bool RelayController::shutdownScreen(bool wait)
    {
        mrRelays.setRelayState(PIN_RELAY_SCREEN_POWER, false);
        ESP_LOGI(mspTag, "Screen relay disabled");
        return true;
    }
}
