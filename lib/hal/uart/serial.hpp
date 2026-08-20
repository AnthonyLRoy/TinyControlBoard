#pragma once

#include "board/boardConfig.hpp"
#include "dataReadyHandshake.hpp"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "heartbeatMonitor.hpp"
#include "protocol/uartProtocol.hpp"
#include "uartRxPump.hpp"

#include <atomic>
#include <functional>

namespace transport::uart
{
    // Thin facade composing the UART peripheral, the data-ready GPIO handshake
    // (DataReadyHandshake), the RX draining task (UartRxPump), and the heartbeat
    // monitor (HeartbeatMonitor) into one hardware singleton.
    class UartTransport
    {
    public:
        static UartTransport &getInstance();

        static void IRAM_ATTR gpioIsrHandler(void *p_arg);

        bool initUart(uart_port_t uartNum,
                      int baudRate,
                      gpio_num_t txPin,
                      gpio_num_t rxPin,
                      size_t bufferSize = 1024,
                      uart_parity_t parity = UART_PARITY_DISABLE,
                      uart_stop_bits_t stopBits = UART_STOP_BITS_1,
                      uart_hw_flowcontrol_t flowCtrl = UART_HW_FLOWCTRL_DISABLE);

        void deinitUart();

        bool sendData(const uint8_t *p_data, size_t len);
        bool sendData(const char *p_message);
        void sendUartCommand(const char *p_logTag, uint32_t commandId);
        void sendUartMessage(const char *p_logTag, UartMessage &rMessage);
        void setRxCallback(std::function<void(const UartMessage &)> callback);

        uint64_t getLastRxTimeUs() const { return m_heartbeat.getLastRxTimeUs(); }

        void startHeartbeatMonitor(uint32_t timeoutMs, std::function<void()> onTimeout);
        void stopHeartbeatMonitor();

    private:
        UartTransport();
        ~UartTransport();

        uart_port_t m_uartNumber;
        std::atomic<bool> m_initialized{false};

        DataReadyHandshake m_handshake;
        UartRxPump m_rxPump;
        HeartbeatMonitor m_heartbeat;
        std::function<void(const UartMessage &)> m_userRxCallback;
    };

    inline constexpr gpio_num_t PIN_RPI_DATA_READY = board::serial::k_rpiDataReadyPin;
    inline constexpr gpio_num_t PIN_ESP32_DATA_READY = board::serial::k_esp32DataReadyPin;

} // namespace transport::uart