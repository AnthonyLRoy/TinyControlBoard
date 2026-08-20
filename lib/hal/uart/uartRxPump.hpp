#pragma once

#include "protocol/uartProtocol.hpp"
#include "uartReceiver.hpp"

#include "driver/uart.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>
#include <functional>

namespace transport::uart
{
    // Owns the UART RX task: drains the UART driver's FIFO, feeds bytes into the
    // framing/checksum layer (UartReceiver), and dispatches complete messages.
    class UartRxPump
    {
    public:
        ~UartRxPump();

        bool begin(uart_port_t uartNum);
        void stop();

        // May be updated at any time, including after begin(); read live by the task.
        void setMessageCallback(std::function<void(const UartMessage &)> callback);

        TaskHandle_t taskHandle() const { return mp_taskHandle.load(std::memory_order_acquire); }
        void notifyFromIsr(BaseType_t *p_higherPriorityTaskWoken);

    private:
        static void taskTrampoline(void *p_arg);
        void run();
        void drainAvailableBytes();

        uart_port_t m_uartNumber = UART_NUM_0;
        // Atomic: written by run() on self-delete, read from stop()/notifyFromIsr() on other tasks/ISR.
        std::atomic<TaskHandle_t> mp_taskHandle{nullptr};
        std::atomic<bool> m_stopTask{false};

        static constexpr size_t k_tmpBufferSize = 64;
        uint8_t m_tmpBuffer[k_tmpBufferSize];
        UartReceiver m_rxBuffer;
        std::function<void(const UartMessage &)> m_onMessage;
    };
} // namespace transport::uart
