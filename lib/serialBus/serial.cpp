#include "Serial.hpp"
#include <cstring>

using namespace serialBus;

static const char *TAG = "SERIAL";

Serial& Serial::instance() {
    static Serial instance;
    return instance;
}

Serial::Serial() : uart_number(UART_NUM_0), initialized(false) {}

Serial::~Serial() {
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
    if (baud_rate <= 0 || uart_num >= UART_NUM_MAX || buffer_size == 0) {
        ESP_LOGE(TAG, "Invalid UART parameters.");
        return false;
    }

    uart_number = uart_num;

    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = parity,
        .stop_bits = stop_bits,
        .flow_ctrl = flow_ctrl,
        .source_clk = UART_SCLK_APB,
    };

    ESP_LOGI(TAG, "Configuring UART%d: %d baud, TX=%d, RX=%d", uart_number, baud_rate, tx_pin, rx_pin); 

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RPI_DATA_READY); 
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    static bool isr_service_installed = false;
    if (!isr_service_installed) {
        gpio_install_isr_service(0); // Call only once globally
        isr_service_installed = true;
    }

    gpio_isr_handler_add(PIN_RPI_DATA_READY, gpio_isr_handler, (void*) this);

    this->task_handle = xTaskGetCurrentTaskHandle();

    ESP_ERROR_CHECK(uart_driver_install(uart_number, buffer_size * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_number, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_number, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    init_data_ready_pin();
    // Launch the RX task
    xTaskCreate([](void* arg) {
        static_cast<Serial*>(arg)->uart_rx_task();
    }, "uart_rx_task", 4096, this, 10, nullptr);

    ESP_LOGI(TAG, "UART%d initialized at %d baud.", uart_number, baud_rate);
    initialized = true;

    set_rx_callback([this](const UARTMessage& msg) {
        ESP_LOGI(TAG, "Received message: cmd=0x%04X", msg.command_id);
        ESP_LOGI(TAG, "Message content: %s", msg.params[0] ? "Non-empty" : "Empty");

});
    return true;
}

void Serial::init_data_ready_pin() {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_ESP32_DATA_READY,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
}
void Serial::deinit_uart() {
    if (initialized) {
        uart_driver_delete(uart_number);
        ESP_LOGI(TAG, "UART%d deinitialized.", uart_number);
        initialized = false;
    }
}

bool Serial::send_data(const char* message) {
    if (!message || !initialized) {
        ESP_LOGW(TAG, "Invalid send attempt.");
        return false;
    }
    ESP_LOGI(TAG, "Sending message: %s", message);
    int written = uart_write_bytes(uart_number, message, strlen(message));
    return written > 0;
}

bool Serial::send_data(const uint8_t* data, size_t len) {
    if (!data || len == 0 || !initialized) {
        ESP_LOGW(TAG, "Invalid send attempt.");
        return false;
    }

    if (gpio_get_level(PIN_ESP32_DATA_READY) == 1) {
        ESP_LOGW(TAG, "Raspberry Pi not ready to receive data.");
        return false;
    }

    ESP_LOGI(TAG, "Sending data of length %zu", len);
    int written = uart_write_bytes(uart_number, data, len);

ESP_LOGI(TAG, "Data sent, signaling Raspberry Pi.");
    ESP_ERROR_CHECK(gpio_set_level(PIN_ESP32_DATA_READY,1)); // Indicate data is ready
    vTaskDelay(pdMS_TO_TICKS(10));       // Small delay
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
        return written == len;
}

void IRAM_ATTR serialBus::Serial::gpio_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    auto *self = static_cast<serialBus::Serial*>(arg);

    if (self->task_handle) {
        vTaskNotifyGiveFromISR(self->task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    ESP_DRAM_LOGD(TAG, "GPIO ISR: notified uart_rx_task");
}

void Serial::handle_uart_rx() {
    if (!initialized) return;

    int len = uart_read_bytes(uart_number, tmp_buffer, TMP_BUFFER_SIZE, UART_PACKET_SIZE / portTICK_PERIOD_MS);
    if (len > 0) {
        rx_buffer.push_bytes(tmp_buffer, len);

        UARTMessage msg;
        while (rx_buffer.get_next_message(msg)) {
            if (rx_callback) {
                ESP_LOGI(TAG, "Parsed message: cmd=0x%04X", msg.command_id);
                rx_callback(msg);
            }
            else {
                ESP_LOGW(TAG, "Failed to parse message");
            }    
        }
    }
}

void Serial::uart_rx_task() {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        handle_uart_rx();
    }
}

void Serial::set_rx_callback(std::function<void(const UARTMessage&)> callback) {
    rx_callback = callback;
}


