#pragma once

#include "board/boardConfig.hpp"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "protocol/uartProtocol.hpp"
#include "transport/uart/uartReceiver.hpp"

#include <functional>

namespace transport::uart
{
    class Serial
    {
    public:
        static Serial &getInstance();

        static void IRAM_ATTR gpioIsrHandler(void *pArg);

        bool initUart(uart_port_t uartNum,
                      int baudRate,
                      gpio_num_t txPin,
                      gpio_num_t rxPin,
                      size_t bufferSize = 1024,
                      uart_parity_t parity = UART_PARITY_DISABLE,
                      uart_stop_bits_t stopBits = UART_STOP_BITS_1,
                      uart_hw_flowcontrol_t flowCtrl = UART_HW_FLOWCTRL_DISABLE);

        void deinitUart();

        bool sendData(const uint8_t *pData, size_t len);
        bool sendData(const char *pMessage);
        void sendUartCommand(const char *pLogTag, uint32_t commandId);
        void sendUartMessage(const char *pLogTag, UartMessage &rMessage);
        void setRxCallback(std::function<void(const UartMessage &)> callback);

        uint64_t getLastRxTimeUs() const { return mLastRxTimeUs; }

        void startHeartbeatMonitor(uint32_t timeoutMs,
                                   std::function<void()> onTimeout);

        void stopHeartbeatMonitor();

    private:
        Serial();
        ~Serial();

        volatile uint64_t mLastRxTimeUs = 0;
        uint32_t mHeartbeatTimeoutMs = 0;
        TaskHandle_t mpHeartbeatTaskHandle = nullptr;
        std::function<void()> mHeartbeatTimeoutCallback = nullptr;

        uart_port_t mUartNumber;
        bool mInitialized;
        TaskHandle_t mpTaskHandle = nullptr;

        static constexpr size_t TMP_BUFFER_SIZE = 64;
        uint8_t mTmpBuffer[TMP_BUFFER_SIZE];
        void initDataReadyPin();
        void initPiDataReadyPin();
        UartReceiver mRxBuffer;
        std::function<void(const UartMessage &)> mRxCallback;

        void runUartRxTask();
        void handleUartRx();
        void onMessageReceived(const UartMessage &rMsg);
    };

    inline constexpr gpio_num_t PIN_RPI_DATA_READY = board::serial::kRpiDataReadyPin;
    inline constexpr gpio_num_t PIN_ESP32_DATA_READY = board::serial::kEsp32DataReadyPin;

} // namespace transport::uart