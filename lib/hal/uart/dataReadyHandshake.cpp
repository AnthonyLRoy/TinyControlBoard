#include "dataReadyHandshake.hpp"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace
{
    constexpr uint32_t k_dataReadySignalHoldMs = 10;
    constexpr const char *k_logTag = "Serial          ";
}

namespace transport::uart
{
    // Configures the Raspberry Pi handshake GPIO as a pulled-down rising-edge input.
    bool DataReadyHandshake::configureRpiInputPin(gpio_num_t pin)
    {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << pin);
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;

        const esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK)
        {
            ESP_LOGE(k_logTag, "Failed to configure RPi data-ready input (err=0x%x)", ret);
            return false;
        }
        return true;
    }

    // Configures the ESP32 handshake GPIO as a low output and stores its pin for later signaling.
    bool DataReadyHandshake::configureEsp32OutputPin(gpio_num_t esp32DataReadyPin)
    {
        m_esp32DataReadyPin = esp32DataReadyPin;

        gpio_config_t io_conf_out = {};
        io_conf_out.pin_bit_mask = (1ULL << esp32DataReadyPin);
        io_conf_out.mode = GPIO_MODE_OUTPUT;
        io_conf_out.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf_out.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_out.intr_type = GPIO_INTR_DISABLE;

        const esp_err_t ret = gpio_config(&io_conf_out);
        if (ret != ESP_OK)
        {
            ESP_LOGE(k_logTag, "Failed to configure ESP32 data-ready output (err=0x%x)", ret);
            return false;
        }
        gpio_set_level(esp32DataReadyPin, 0);
        return true;
    }

    // Installs the GPIO ISR service once and attaches the receive-wakeup handler to the Pi input pin.
    bool DataReadyHandshake::installIsrHandler(gpio_num_t rpiDataReadyPin, gpio_isr_t handler, void *p_arg)
    {
        static bool s_isrServiceInstalled = false;
        if (!s_isrServiceInstalled)
        {
            const esp_err_t ret = gpio_install_isr_service(0);
            if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
            {
                ESP_LOGE(k_logTag, "Failed to install ISR service: %d", ret);
                return false;
            }
            s_isrServiceInstalled = true;
        }

        const esp_err_t ret = gpio_isr_handler_add(rpiDataReadyPin, handler, p_arg);
        if (ret != ESP_OK)
        {
            ESP_LOGE(k_logTag, "Failed to add GPIO ISR handler (err=0x%x)", ret);
            return false;
        }
        return true;
    }

    // Checks whether the ESP32 handshake output is currently asserted.
    bool DataReadyHandshake::isPulseInProgress() const
    {
        return gpio_get_level(m_esp32DataReadyPin) == 1;
    }

    // Pulses the ESP32 handshake output high for the required hold time, then returns it low.
    void DataReadyHandshake::pulseDataReady() const
    {
        gpio_set_level(m_esp32DataReadyPin, 1);
        vTaskDelay(pdMS_TO_TICKS(k_dataReadySignalHoldMs));
        gpio_set_level(m_esp32DataReadyPin, 0);
    }
} // namespace transport::uart
