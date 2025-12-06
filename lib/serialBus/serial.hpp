#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "uart_protocol.hpp"
#include "UartReceiver.hpp"
#include <functional>

namespace serialBus
{

    class Serial
    {
    public:
        static Serial &instance();

        static void IRAM_ATTR gpio_isr_handler(void *arg);

        bool init_uart(uart_port_t uart_num,
                       int baud_rate,
                       gpio_num_t tx_pin,
                       gpio_num_t rx_pin,
                       size_t buffer_size = 1024,
                       uart_parity_t parity = UART_PARITY_DISABLE,
                       uart_stop_bits_t stop_bits = UART_STOP_BITS_1,
                       uart_hw_flowcontrol_t flow_ctrl = UART_HW_FLOWCTRL_DISABLE);

        void deinit_uart();

        bool send_data(const uint8_t *data, size_t len);
        bool send_data(const char *message);

        void set_rx_callback(std::function<void(const UARTMessage &)> callback);

        // Heartbeat monitoring
        uint64_t get_last_rx_time_us() const { return last_rx_time_us; }

        void start_heartbeat_monitor(uint32_t timeout_ms,
                                     std::function<void()> on_timeout);

        void stop_heartbeat_monitor();

    private:
        Serial();
        ~Serial();

        // Heartbeat monitoring
        volatile uint64_t last_rx_time_us = 0;
        uint32_t heartbeat_timeout_ms = 0;
        TaskHandle_t heartbeat_task_handle = nullptr;
        std::function<void()> heartbeat_timeout_callback = nullptr;

        uart_port_t uart_number;
        bool initialized;
        TaskHandle_t task_handle = nullptr;

        static constexpr size_t TMP_BUFFER_SIZE = 64;
        uint8_t tmp_buffer[TMP_BUFFER_SIZE];
        void init_data_ready_pin();
        void init_piData_ready_pin();
        UartReceiver rx_buffer;
        std::function<void(const UARTMessage &)> rx_callback;

        void uart_rx_task();
        void handle_uart_rx();
        void on_message_received(const UARTMessage &msg);
    };

    // GPIO from Raspberry Pi indicating data available

    // Fires when Pi sets this pin HIGH
    static constexpr gpio_num_t PIN_RPI_DATA_READY = GPIO_NUM_42;

    // GPIO from ESP32 indicating data available raised High by ESP32 //
    static constexpr gpio_num_t PIN_ESP32_DATA_READY = GPIO_NUM_41;

} // namespace serialBus
