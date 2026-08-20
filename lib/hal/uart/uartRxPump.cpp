#include "uartRxPump.hpp"

#include <esp_log.h>

namespace
{
    constexpr uint32_t k_taskStackSize = 4096;
    constexpr uint32_t k_taskPriority = 10;
    constexpr const char *k_logTag = "Serial          ";
}

namespace transport::uart
{
    UartRxPump::~UartRxPump()
    {
        stop();
    }

    bool UartRxPump::begin(uart_port_t uartNum)
    {
        m_uartNumber = uartNum;

        if (mp_taskHandle.load(std::memory_order_acquire) != nullptr)
        {
            return true;
        }

        m_stopTask.store(false, std::memory_order_release);
        TaskHandle_t handle = nullptr;
        if (xTaskCreate(taskTrampoline, "uart_rx_task", k_taskStackSize, this, k_taskPriority, &handle) != pdPASS)
        {
            ESP_LOGE(k_logTag, "Failed to create UART RX task");
            return false;
        }
        mp_taskHandle.store(handle, std::memory_order_release);
        return true;
    }

    void UartRxPump::stop()
    {
        m_stopTask.store(true, std::memory_order_release);
        TaskHandle_t handle = mp_taskHandle.load(std::memory_order_acquire);
        if (handle == nullptr)
        {
            return;
        }

        xTaskNotifyGive(handle);
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            for (int i = 0; mp_taskHandle.load(std::memory_order_acquire) && i < 50; ++i)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        if (mp_taskHandle.load(std::memory_order_acquire))
        {
            ESP_LOGW(k_logTag, "UART RX task did not stop in time; forcing delete");
            vTaskDelete(handle);
            mp_taskHandle.store(nullptr, std::memory_order_release);
        }
    }

    void UartRxPump::setMessageCallback(std::function<void(const UartMessage &)> callback)
    {
        m_onMessage = std::move(callback);
    }

    void UartRxPump::notifyFromIsr(BaseType_t *p_higherPriorityTaskWoken)
    {
        TaskHandle_t handle = mp_taskHandle.load(std::memory_order_acquire);
        if (handle)
        {
            vTaskNotifyGiveFromISR(handle, p_higherPriorityTaskWoken);
        }
    }

    void UartRxPump::taskTrampoline(void *p_arg)
    {
        static_cast<UartRxPump *>(p_arg)->run();
    }

    void UartRxPump::run()
    {
        while (!m_stopTask.load(std::memory_order_acquire))
        {
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
            if (m_stopTask.load(std::memory_order_acquire))
            {
                break;
            }
            drainAvailableBytes();
        }

        mp_taskHandle.store(nullptr, std::memory_order_release);
        vTaskDelete(nullptr);
    }

    void UartRxPump::drainAvailableBytes()
    {
        // A burst of many packets (e.g. a library listing) can arrive faster than this
        // task is woken — GPIO wake notifications coalesce into a single wake-up — so
        // drain everything currently buffered rather than reading one chunk.
        TickType_t waitTicks = UART_PACKET_SIZE / portTICK_PERIOD_MS;
        int receivedDataLength;
        while ((receivedDataLength = uart_read_bytes(m_uartNumber, m_tmpBuffer, k_tmpBufferSize, waitTicks)) > 0)
        {
            m_rxBuffer.pushBytes(m_tmpBuffer, receivedDataLength);

            UartMessage msg;
            while (m_rxBuffer.getNextMessage(msg))
            {
                if (m_onMessage)
                {
                    m_onMessage(msg);
                }
                else
                {
                    ESP_LOGW(k_logTag, "RX callback not set; dropping parsed message");
                }
            }

            waitTicks = 0; // further reads this wake only drain what's already buffered
        }
    }
} // namespace transport::uart
