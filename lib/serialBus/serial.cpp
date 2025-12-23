#include "Serial.hpp"
#include <cstring>
#include <esp_timer.h>

using namespace serialBus;

static const char *TAG = "SERIAL";

Serial &Serial::instance()
{
    static Serial instance;
    return instance;
}

Serial::Serial() : uart_number(UART_NUM_0), initialized(false) {}

Serial::~Serial()
{
    deinit_uart();
}

bool Serial::init_uart(uart_port_t uart_num,
                       int baud_rate,
                       gpio_num_t tx_pin,
                       gpio_num_t rx_pin,
                       size_t buffer_size,
                       uart_parity_t parity,
                       uart_stop_bits_t stop_bits,
                       uart_hw_flowcontrol_t flow_ctrl)
{
    if (baud_rate <= 0 || uart_num >= UART_NUM_MAX || buffer_size == 0)
    {
        ESP_LOGE(TAG, "Invalid UART parameters.");
        return false;
    }

    ESP_LOGI(TAG, "Initializing UART%d...", uart_num);
    uart_number = uart_num;

    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = parity,
        .stop_bits = stop_bits,
        .flow_ctrl = flow_ctrl,
        .source_clk = UART_SCLK_APB,
    };

    ESP_LOGI(TAG, "Configuring UART%d: %d baud, TX=%d, RX=%d", uart_number, baud_rate, tx_pin, rx_pin);

    // -------------------------
    // 1. Configure RPi Data Ready pin (input with interrupt)
    // -------------------------
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE; // Rising edge
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RPI_DATA_READY);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; // Depends on wiring
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    // -------------------------
    // 2. Create RX task BEFORE adding ISR
    // -------------------------
    if (!initialized)
    {
        xTaskCreate([](void *arg)
                    { static_cast<Serial *>(arg)->uart_rx_task(); }, "uart_rx_task", 4096, this, 10, &this->task_handle);
    }

    // -------------------------
    // 3. Install ISR service (only once globally)
    // -------------------------
    static bool isr_service_installed = false;
    if (!isr_service_installed)
    {
        esp_err_t ret = gpio_install_isr_service(0);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(TAG, "Failed to install ISR service: %d", ret);
            return false;
        }
        isr_service_installed = true;
    }

    // -------------------------
    // 4. Attach ISR handler
    // -------------------------
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_RPI_DATA_READY, gpio_isr_handler, (void *)this));

    // -------------------------
    // 5. Initialize ESP32 Data Ready pin (output, signaling to RPi)
    // -------------------------
    gpio_config_t io_conf_out = {};
    io_conf_out.pin_bit_mask = (1ULL << PIN_ESP32_DATA_READY);
    io_conf_out.mode = GPIO_MODE_OUTPUT;
    io_conf_out.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf_out.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_out.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf_out);
    gpio_set_level(PIN_ESP32_DATA_READY, 0);

    // -------------------------
    // 6. UART driver setup
    // -------------------------
    ESP_ERROR_CHECK(uart_driver_install(uart_number, buffer_size * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_number, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_number, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART%d initialized at %d baud.", uart_number, baud_rate);
    initialized = true;

    // Optional: set RX callback
    // set_rx_callback([this](const UARTMessage &msg)
    //                 {
                              


    //                      ESP_LOGI(TAG, "Received message: cmd=0x%04X", msg.command_id);
    //                 });

    return true;
}

void Serial::init_data_ready_pin()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_ESP32_DATA_READY,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
}

void Serial::deinit_uart()
{
    if (initialized)
    {
        uart_driver_delete(uart_number);
        ESP_LOGI(TAG, "UART%d deinitialized.", uart_number);
        initialized = false;
    }
}

void Serial::sendUartMessage(const char *logTag,  UARTMessage &message)
{
    uint8_t tx_buffer[UART_PACKET_SIZE];
    serialize_message(message, tx_buffer);

    ESP_LOGI(logTag, "Sending %s message (cmd=0x%04X)", logTag, message.command_id);

    if (!send_data(tx_buffer, UART_PACKET_SIZE))
    {
        ESP_LOGE(logTag, "Failed to send %s message", logTag);
    }
    else
    {
        ESP_LOGI(logTag, "%s message sent successfully", logTag);
    }
}

bool Serial::send_data(const uint8_t *data, size_t len)
{
    if (!data || len == 0 || !initialized)
    {
        ESP_LOGW(TAG, "Invalid send attempt.");
        return false;
    }

    if (gpio_get_level(PIN_ESP32_DATA_READY) == 1)
    {
        ESP_LOGW(TAG, "Raspberry Pi not ready to receive data.");
        return false;
    }

    ESP_LOGI(TAG, "Sending data of length %zu", len);
    int written = uart_write_bytes(uart_number, data, len);

    ESP_LOGI(TAG, "Data sent, signaling Raspberry Pi.");
    ESP_ERROR_CHECK(gpio_set_level(PIN_ESP32_DATA_READY, 1)); // Indicate data is ready
    vTaskDelay(pdMS_TO_TICKS(10));                            // Small delay
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
    return written == len;
}

void Serial::sendUartCommand(const char *logTag, uint32_t commandId)
{
    UARTMessage msg{};
    msg.command_id = commandId;
    sendUartMessage(logTag, msg);
}

void IRAM_ATTR Serial::gpio_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    auto *self = static_cast<Serial *>(arg);

    if (self->task_handle)
    {
        // Notify RX task from ISR
        vTaskNotifyGiveFromISR(self->task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void Serial::handle_uart_rx()
{
    if (initialized)
    {
        int receivedDataLength = uart_read_bytes(uart_number, tmp_buffer, TMP_BUFFER_SIZE, UART_PACKET_SIZE / portTICK_PERIOD_MS);

        if (receivedDataLength > 0)
        {
            // ESP_LOGI(TAG, "UART RX: Read %d bytes", len);
            rx_buffer.push_bytes(tmp_buffer, receivedDataLength);

            UARTMessage msg;
            while (rx_buffer.get_next_message(msg))
            {
                if (rx_callback)
                {
                    last_rx_time_us = esp_timer_get_time();
                    rx_callback(msg);
                }
                else
                {
                    ESP_LOGW(TAG, "Failed to parse message");
                }
            }
        }
    }
    else
    {
        ESP_LOGW(TAG, "UART not initialized, cannot handle RX");
    }
}

void Serial::start_heartbeat_monitor(uint32_t timeout_ms,
                                     std::function<void()> on_timeout)
{
    heartbeat_timeout_ms = timeout_ms;
    heartbeat_timeout_callback = on_timeout;

    // Initialize last_rx_time_us to current time so timeout begins immediately
    last_rx_time_us = esp_timer_get_time();

    if (heartbeat_task_handle == nullptr)
    {
        xTaskCreate(
            [](void *arg)
            {
                Serial *self = static_cast<Serial *>(arg);
                const TickType_t delay = pdMS_TO_TICKS(100);

                while (true)
                {
                    vTaskDelay(delay);

                    uint64_t now = esp_timer_get_time();
                    uint64_t last = self->last_rx_time_us;

                    if (last == 0)
                    {
                        // No messages yet — do nothing
                        continue;
                    }

                    uint64_t diff_ms = (now - last) / 1000;

                    if (diff_ms > self->heartbeat_timeout_ms)
                    {
                        // Trigger callback ONCE
                        if (self->heartbeat_timeout_callback)
                            self->heartbeat_timeout_callback();

                        // Reset timestamp so callback fires only once
                        self->last_rx_time_us = now;
                    }
                }
            },
            "heartbeat_task",
            4096,
            this,
            5,
            &heartbeat_task_handle);
    }
}

void Serial::stop_heartbeat_monitor()
{
    if (heartbeat_task_handle)
    {
        vTaskDelete(heartbeat_task_handle);
        heartbeat_task_handle = nullptr;
    }
}

void Serial::uart_rx_task()
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        handle_uart_rx();
    }
}

void Serial::set_rx_callback(std::function<void(const UARTMessage &)> callback)
{
    rx_callback = callback;
}
