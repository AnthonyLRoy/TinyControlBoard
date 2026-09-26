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
        // Returns the singleton UART transport instance used by the board.
        static UartTransport &getInstance();

        // ISR hook that wakes the RX pump when the Raspberry Pi asserts the data-ready pin.
        static void IRAM_ATTR gpioIsrHandler(void *p_arg);

        // Initializes the selected UART port, pins, and RX pump for board-to-RPi communication.
        bool initUart(uart_port_t uartNum,
                      int baudRate,
                      gpio_num_t txPin,
                      gpio_num_t rxPin,
                      size_t bufferSize = 1024,
                      uart_parity_t parity = UART_PARITY_DISABLE,
                      uart_stop_bits_t stopBits = UART_STOP_BITS_1,
                      uart_hw_flowcontrol_t flowCtrl = UART_HW_FLOWCTRL_DISABLE);

        // Stops the receive pump and tears down the UART driver during shutdown.
        void deinitUart();

        // Sends raw bytes over the UART link when the serial transport is initialized.
        bool sendData(const uint8_t *p_data, size_t len);
        // Sends a null-terminated text payload using the UART transport.
        bool sendData(const char *p_message);
        // Sends a command ID without any additional payload data.
        void sendUartCommand(const char *p_logTag, uint32_t commandId);
        // Serializes and transmits a complete UART message object.
        void sendUartMessage(const char *p_logTag, UartMessage &rMessage);
        // Registers the callback that receives each decoded UART message.
        void setRxCallback(std::function<void(const UartMessage &)> callback);

        // Returns the timestamp of the most recent valid RX event used by the heartbeat monitor.
        uint64_t getLastRxTimeUs() const { return m_heartbeat.getLastRxTimeUs(); }

        // Starts the watchdog that monitors UART traffic and triggers a timeout callback.
        void startHeartbeatMonitor(uint32_t timeoutMs, std::function<void()> onTimeout);
        // Stops the heartbeat monitor and clears any active timeout task state.
        void stopHeartbeatMonitor();

    private:
        // Creates the singleton transport facade and its UART helper components.
        UartTransport();
        // Stops the UART services and releases transport resources.
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