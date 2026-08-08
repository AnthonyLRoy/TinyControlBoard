#include "power/RelayController.hpp"

#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace controlSystem
{
    RelayController::RelayController(transport::uart::UartTransport &rSerial, relays::StandardRelay &rRelays)
        : mr_serial(rSerial), mr_relays(rRelays)
    {
    }

    void RelayController::setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs)
    {
        ESP_LOGI(k_logTag, "Setting relay %d to %s with delay %lu ms", pin, state ? "ON" : "OFF", delayMs);
        mr_relays.setRelayState(pin, state);
        if (delayMs > 0) {
            vTaskDelay(pdMS_TO_TICKS(delayMs));
        }
    }

    bool RelayController::handleToggleDac(bool state)
    {
        // Toggle DAC selects between DAC signal outputs (GPIO10); it does not gate DAC power (GPIO12).
        mr_relays.setRelayState(PIN_RELAY_PROTO_DAC_ENABLED, state);
        m_dacEnabled = state;
        ESP_LOGI(k_logTag, "DAC output select set to %s", state ? "ON" : "OFF");
        return true;
    }

    bool RelayController::toggleDac()
    {
        return handleToggleDac(!m_dacEnabled);
    }

    bool RelayController::shutdownRpi()
    {
        mr_relays.setRelayState(PIN_RELAY_RPI_POWER, false);
        ESP_LOGI(k_logTag, "RPI relay disabled");
        return true;
    }

    bool RelayController::shutdownScreen()
    {
        mr_relays.setRelayState(PIN_RELAY_SCREEN_POWER, false);
        ESP_LOGI(k_logTag, "Screen relay disabled");
        return true;
    }
}