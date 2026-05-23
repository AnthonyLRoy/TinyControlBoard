#include "transport/uart/serial.hpp"

#include <cstring>
#include <esp_timer.h>

using namespace transport::uart;

namespace
{
    constexpr uint32_t k_dataReadySignalHoldMs = 10;
    constexpr uint32_t k_heartbeatCheckIntervalMs = 100;
    constexpr uint32_t k_uartRxTaskStackSize = 4096;
    constexpr uint32_t k_uartRxTaskPriority = 10;
    constexpr uint32_t k_heartbeatTaskStackSize = 4096;
    constexpr uint32_t k_heartbeatTaskPriority = 5;
}

static constexpr const char *k_logTag = "Serial          ";

UartTransport &UartTransport::getInstance()
{
    static UartTransport s_instance;
    return s_instance;
}

UartTransport::UartTransport() : m_uartNumber(UART_NUM_0) {}

UartTransport::~UartTransport()
{
    deinitUart();
}

bool UartTransport::initUart(uart_port_t uartNum,
                      int baudRate,
                      gpio_num_t txPin,
                      gpio_num_t rxPin,
                      size_t bufferSize,
                      uart_parity_t parity,
                      uart_stop_bits_t stopBits,
                      uart_hw_flowcontrol_t flowCtrl)
{
    if (baudRate <= 0 || uartNum >= UART_NUM_MAX || bufferSize == 0)
    {
        ESP_LOGE(k_logTag, "Invalid UART parameters.");
        return false;
    }

    ESP_LOGI(k_logTag, "Initializing UART%d...", uartNum);
    m_uartNumber = uartNum;

    uart_config_t uart_config = {
        .baud_rate = baudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = parity,
        .stop_bits = stopBits,
        .flow_ctrl = flowCtrl,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };

    ESP_LOGI(k_logTag, "Configuring UART%d: %d baud, TX=%d, RX=%d", m_uartNumber, baudRate, txPin, rxPin);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RPI_DATA_READY);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to configure RPi data-ready input (err=0x%x)", ret);
        return false;
    }

    if (!m_initialized.load(std::memory_order_acquire))
    {
        m_stopRxTask.store(false, std::memory_order_release);
        if (xTaskCreate([](void *arg)
                        { static_cast<UartTransport *>(arg)->runUartRxTask(); },
                        "uart_rx_task",
                        k_uartRxTaskStackSize,
                        this,
                        k_uartRxTaskPriority,
                        &this->mp_taskHandle) != pdPASS)
        {
            mp_taskHandle = nullptr;
            ESP_LOGE(k_logTag, "Failed to create UART RX task");
            return false;
        }
    }

    static bool s_isrServiceInstalled = false;
    if (!s_isrServiceInstalled)
    {
        ret = gpio_install_isr_service(0);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(k_logTag, "Failed to install ISR service: %d", ret);
            return false;
        }
        s_isrServiceInstalled = true;
    }

    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_RPI_DATA_READY, gpioIsrHandler, (void *)this));

    gpio_config_t io_conf_out = {};
    io_conf_out.pin_bit_mask = (1ULL << PIN_ESP32_DATA_READY);
    io_conf_out.mode = GPIO_MODE_OUTPUT;
    io_conf_out.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf_out.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_out.intr_type = GPIO_INTR_DISABLE;
    ret = gpio_config(&io_conf_out);
    if (ret != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to configure ESP32 data-ready output (err=0x%x)", ret);
        return false;
    }
    gpio_set_level(PIN_ESP32_DATA_READY, 0);

    ESP_ERROR_CHECK(uart_driver_install(m_uartNumber, bufferSize * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(m_uartNumber, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(m_uartNumber, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(k_logTag, "UART%d initialized at %d baud.", m_uartNumber, baudRate);
    m_initialized.store(true, std::memory_order_release);
    return true;
}

void UartTransport::initDataReadyPin()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_ESP32_DATA_READY,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    const esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to initialize data-ready pin (err=0x%x)", err);
        return;
    }
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
}

void UartTransport::deinitUart()
{
    stopHeartbeatMonitor();

    m_stopRxTask.store(true, std::memory_order_release);
    if (mp_taskHandle)
    {
        xTaskNotifyGive(mp_taskHandle);
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            for (int i = 0; mp_taskHandle && i < 50; ++i)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        if (mp_taskHandle)
        {
            ESP_LOGW(k_logTag, "UART RX task did not stop in time; forcing delete");
            vTaskDelete(mp_taskHandle);
            mp_taskHandle = nullptr;
        }
    }

    if (m_initialized.load(std::memory_order_acquire))
    {
        uart_driver_delete(m_uartNumber);
        ESP_LOGI(k_logTag, "UART%d deinitialized.", m_uartNumber);
        m_initialized.store(false, std::memory_order_release);
    }
}

void UartTransport::sendUartMessage(const char *p_logTag, UartMessage &rMessage)
{
    uint8_t txBuffer[UART_PACKET_SIZE];
    serializeMessage(rMessage, txBuffer);

    ESP_LOGI(p_logTag, "Sending %s message (cmd=0x%04X)", p_logTag, rMessage.commandId);

    if (!sendData(txBuffer, UART_PACKET_SIZE))
    {
        ESP_LOGE(p_logTag, "Failed to send %s message", p_logTag);
    }
    else
    {
        ESP_LOGI(p_logTag, "%s message sent successfully", p_logTag);
    }
}

bool UartTransport::sendData(const uint8_t *p_data, size_t len)
{
    if (!p_data || len == 0 || !m_initialized.load(std::memory_order_acquire))
    {
        ESP_LOGW(k_logTag, "Invalid send attempt");
        return false;
    }

    if (gpio_get_level(PIN_ESP32_DATA_READY) == 1)
    {
        ESP_LOGW(k_logTag, "Raspberry Pi not ready to receive data");
        return false;
    }

    ESP_LOGI(k_logTag, "Sending data of length %zu", len);
    int written = uart_write_bytes(m_uartNumber, p_data, len);

    ESP_LOGI(k_logTag, "Data sent, signaling Raspberry Pi.");
    ESP_ERROR_CHECK(gpio_set_level(PIN_ESP32_DATA_READY, 1));
    vTaskDelay(pdMS_TO_TICKS(k_dataReadySignalHoldMs));
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
    return written == len;
}

void UartTransport::sendUartCommand(const char *p_logTag, uint32_t commandId)
{
    UartMessage msg{};
    msg.commandId = commandId;
    sendUartMessage(p_logTag, msg);
}

void IRAM_ATTR UartTransport::gpioIsrHandler(void *p_arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    auto *p_self = static_cast<UartTransport *>(p_arg);

    if (p_self->mp_taskHandle)
    {
        vTaskNotifyGiveFromISR(p_self->mp_taskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void UartTransport::handleUartRx()
{
    if (m_initialized.load(std::memory_order_acquire))
    {
        int receivedDataLength = uart_read_bytes(m_uartNumber, m_tmpBuffer, TMP_BUFFER_SIZE, UART_PACKET_SIZE / portTICK_PERIOD_MS);

        if (receivedDataLength > 0)
        {
            m_rxBuffer.pushBytes(m_tmpBuffer, receivedDataLength);

            UartMessage msg;
            while (m_rxBuffer.getNextMessage(msg))
            {
                if (m_rxCallback)
                {
                    m_lastRxTimeUs.store(esp_timer_get_time(), std::memory_order_relaxed);
                    m_rxCallback(msg);
                }
                else
                {
                    ESP_LOGW(k_logTag, "RX callback not set; dropping parsed message");
                }
            }
        }
    }
    else
    {
        ESP_LOGW(k_logTag, "UART not initialized; cannot handle RX");
    }
}

void UartTransport::startHeartbeatMonitor(uint32_t timeoutMs,
                                   std::function<void()> onTimeout)
{
    m_heartbeatTimeoutMs = timeoutMs;
    m_heartbeatTimeoutCallback = onTimeout;
    m_lastRxTimeUs.store(esp_timer_get_time(), std::memory_order_relaxed);

    if (mp_heartbeatTaskHandle == nullptr)
    {
        m_stopHeartbeatTask.store(false, std::memory_order_release);
        if (xTaskCreate(
            [](void *arg)
            {
                UartTransport *p_self = static_cast<UartTransport *>(arg);
                const TickType_t delay = pdMS_TO_TICKS(k_heartbeatCheckIntervalMs);

                while (!p_self->m_stopHeartbeatTask.load(std::memory_order_acquire))
                {
                    vTaskDelay(delay);

                    if (p_self->m_stopHeartbeatTask.load(std::memory_order_acquire))
                    {
                        break;
                    }

                    uint64_t now = esp_timer_get_time();
                    uint64_t last = p_self->m_lastRxTimeUs.load(std::memory_order_relaxed);

                    if (last == 0)
                    {
                        continue;
                    }

                    uint64_t diff_ms = (now - last) / 1000;

                    if (diff_ms > p_self->m_heartbeatTimeoutMs)
                    {
                        if (p_self->m_heartbeatTimeoutCallback)
                            p_self->m_heartbeatTimeoutCallback();

                        p_self->m_lastRxTimeUs.store(now, std::memory_order_relaxed);
                    }
                }

                p_self->mp_heartbeatTaskHandle = nullptr;
                vTaskDelete(nullptr);
            },
            "heartbeat_task",
            k_heartbeatTaskStackSize,
            this,
            k_heartbeatTaskPriority,
            &mp_heartbeatTaskHandle) != pdPASS)
        {
            mp_heartbeatTaskHandle = nullptr;
            ESP_LOGE(k_logTag, "Failed to create heartbeat monitor task");
        }
    }
}

void UartTransport::stopHeartbeatMonitor()
{
    if (mp_heartbeatTaskHandle)
    {
        m_stopHeartbeatTask.store(true, std::memory_order_release);
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            for (int i = 0; mp_heartbeatTaskHandle && i < 50; ++i)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        if (mp_heartbeatTaskHandle)
        {
            ESP_LOGW(k_logTag, "Heartbeat task did not stop in time; forcing delete");
            vTaskDelete(mp_heartbeatTaskHandle);
            mp_heartbeatTaskHandle = nullptr;
        }
    }
}

void UartTransport::runUartRxTask()
{
    while (!m_stopRxTask.load(std::memory_order_acquire))
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
        if (m_stopRxTask.load(std::memory_order_acquire))
        {
            break;
        }
        handleUartRx();
    }

    mp_taskHandle = nullptr;
    vTaskDelete(nullptr);
}

void UartTransport::setRxCallback(std::function<void(const UartMessage &)> callback)
{
    m_rxCallback = callback;
}