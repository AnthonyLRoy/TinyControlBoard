#pragma once

#include "board/boardConfig.hpp"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "protocol/uartProtocol.hpp"
#include "hal/uart/UartCommandSink.hpp"
#include "uartReceiver.hpp"

#include <atomic>
#include <functional>

namespace transport::uart
{
    class UartTransport : public IUartCommandSink
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
        void sendUartCommand(const char *p_logTag, uint32_t commandId) override;
        void sendUartMessage(const char *p_logTag, UartMessage &rMessage) override;
        void setRxCallback(std::function<void(const UartMessage &)> callback);

        uint64_t getLastRxTimeUs() const { return m_lastRxTimeUs.load(std::memory_order_relaxed); }

        void startHeartbeatMonitor(uint32_t timeoutMs,
                                   std::function<void()> onTimeout);

        void stopHeartbeatMonitor();

    private:
        UartTransport();
        ~UartTransport();

        std::atomic<uint64_t> m_lastRxTimeUs{0};
        uint32_t m_heartbeatTimeoutMs = 0;
        TaskHandle_t mp_heartbeatTaskHandle = nullptr;
        std::function<void()> m_heartbeatTimeoutCallback = nullptr;

        uart_port_t m_uartNumber;
        std::atomic<bool> m_initialized{false};
        std::atomic<bool> m_stopRxTask{false};
        std::atomic<bool> m_stopHeartbeatTask{false};
        TaskHandle_t mp_taskHandle = nullptr;

        static constexpr size_t TMP_BUFFER_SIZE = 64;
        uint8_t m_tmpBuffer[TMP_BUFFER_SIZE];
        void initDataReadyPin();
        void initPiDataReadyPin();
        UartReceiver m_rxBuffer;
        std::function<void(const UartMessage &)> m_rxCallback;

        void runUartRxTask();
        void handleUartRx();
        void onMessageReceived(const UartMessage &rMsg);
    };

} // namespace transport::uart