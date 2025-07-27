#pragma once
#include <driver/gpio.h>
#include "driver/uart.h"
#include "esp_log.h"
#include <cstring>
#include "uart_protocol.hpp"

namespace serialBus {

class Serial {
public:
    Serial();
    ~Serial();

    bool init_uart(uart_port_t uart_num,
                   int baud_rate,
                   gpio_num_t tx_pin,
                   gpio_num_t rx_pin,
                   size_t buffer_size = 1024,
                   uart_parity_t parity = UART_PARITY_DISABLE,
                   uart_stop_bits_t stop_bits = UART_STOP_BITS_1,
                   uart_hw_flowcontrol_t flow_ctrl = UART_HW_FLOWCTRL_DISABLE);

    void deinit_uart();
    bool send_data(const char *message);
    int read_data(char *buffer, size_t buffer_size, TickType_t timeout_ms = 100);

private:
    uart_port_t uart_number;
    bool initialized;
};

} // namespace serialbus
