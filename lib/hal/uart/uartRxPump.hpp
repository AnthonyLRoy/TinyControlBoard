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
        // Cleans up the RX task and associated buffers when the pump is destroyed.
        ~UartRxPump();

        // Starts the UART receive task and connects it to the selected UART peripheral.
        bool begin(uart_port_t uartNum);
        // Stops the receive task and tears down any queued processing.
        void stop();

        // May be updated at any time, including after begin(); read live by the task.
        // Sets the callback that receives each fully decoded message from the RX stream.
        void setMessageCallback(std::function<void(const UartMessage &)> callback);

        // Returns the handle of the current RX task, if one is running.
        TaskHandle_t taskHandle() const { return mp_taskHandle.load(std::memory_order_acquire); }
        // Wakes the RX task from ISR context when the UART data-ready line triggers.
        void notifyFromIsr(BaseType_t *p_higherPriorityTaskWoken);

    private:
        // FreeRTOS trampoline that calls the instance method for the receive task.
        static void taskTrampoline(void *p_arg);
        // Runs the UART receive loop and feeds bytes into the message decoder.
        void run();
        // Drains any bytes currently available in the UART driver FIFO.
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
