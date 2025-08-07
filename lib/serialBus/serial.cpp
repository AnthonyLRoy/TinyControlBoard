#include "Serial.hpp"

static const char *TAG = "SERIAL";


serialBus::Serial& serialBus::Serial::instance() {
    static Serial instance;
    return instance;
}

serialBus::Serial::Serial() : uart_number(UART_NUM_1), initialized(false) {}

serialBus::Serial::~Serial() {
    deinit_uart();
}

bool serialBus::Serial::init_uart(uart_port_t uart_num,
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

    ESP_ERROR_CHECK(uart_driver_install(uart_number, buffer_size * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_number, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_number, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART%d initialized at %d baud.", uart_number, baud_rate);
    initialized = true;
    return true;
}

void serialBus::Serial::deinit_uart() {
    if (initialized) {
        uart_driver_delete(uart_number);
        ESP_LOGI(TAG, "UART%d deinitialized.", uart_number);
        initialized = false;
    }
}

bool serialBus::Serial::send_data(const char *message) {
    if (!message || !initialized) {
        ESP_LOGW(TAG, "Invalid send attempt.");
        return false;
    }

    int written = uart_write_bytes(uart_number, message, strlen(message));
    return written > 0;
}

int serialBus::Serial::read_data(char *buffer, size_t buffer_size, TickType_t timeout_ms) {
    if (!buffer || buffer_size < 2 || !initialized) {
        ESP_LOGE(TAG, "Invalid buffer or UART not initialized.");
        return 0;
    }

    int len = uart_read_bytes(uart_number, (uint8_t *)buffer, buffer_size - 1, timeout_ms / portTICK_PERIOD_MS);
    if (len > 0) {
        buffer[len] = '\0';
    } else {
        buffer[0] = '\0';
    }

    return len;
}
