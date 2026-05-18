#include "transport/uart/serial.hpp"

#include <cstring>
#include <esp_timer.h>

using namespace transport::uart;

namespace
{
    constexpr uint32_t kDataReadySignalHoldMs = 10;
    constexpr uint32_t kHeartbeatCheckIntervalMs = 100;
    constexpr uint32_t kUartRxTaskStackSize = 4096;
    constexpr uint32_t kUartRxTaskPriority = 10;
    constexpr uint32_t kHeartbeatTaskStackSize = 4096;
    constexpr uint32_t kHeartbeatTaskPriority = 5;
}

static const char *spTag = "Serial          ";

UartTransport &UartTransport::getInstance()
{
    static UartTransport sInstance;
    return sInstance;
}

UartTransport::UartTransport() : mUartNumber(UART_NUM_0), mInitialized(false) {}

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
        ESP_LOGE(spTag, "Invalid UART parameters.");
        return false;
    }

    ESP_LOGI(spTag, "Initializing UART%d...", uartNum);
    mUartNumber = uartNum;

    uart_config_t uart_config = {
        .baud_rate = baudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = parity,
        .stop_bits = stopBits,
        .flow_ctrl = flowCtrl,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };

    ESP_LOGI(spTag, "Configuring UART%d: %d baud, TX=%d, RX=%d", mUartNumber, baudRate, txPin, rxPin);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RPI_DATA_READY);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    if (!mInitialized)
    {
        xTaskCreate([](void *arg)
                    { static_cast<UartTransport *>(arg)->runUartRxTask(); },
                    "uart_rx_task",
                    kUartRxTaskStackSize,
                    this,
                    kUartRxTaskPriority,
                    &this->mpTaskHandle);
    }

    static bool sIsrServiceInstalled = false;
    if (!sIsrServiceInstalled)
    {
        esp_err_t ret = gpio_install_isr_service(0);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(spTag, "Failed to install ISR service: %d", ret);
            return false;
        }
        sIsrServiceInstalled = true;
    }

    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_RPI_DATA_READY, gpioIsrHandler, (void *)this));

    gpio_config_t io_conf_out = {};
    io_conf_out.pin_bit_mask = (1ULL << PIN_ESP32_DATA_READY);
    io_conf_out.mode = GPIO_MODE_OUTPUT;
    io_conf_out.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf_out.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_out.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf_out);
    gpio_set_level(PIN_ESP32_DATA_READY, 0);

    ESP_ERROR_CHECK(uart_driver_install(mUartNumber, bufferSize * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(mUartNumber, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(mUartNumber, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(spTag, "UART%d initialized at %d baud.", mUartNumber, baudRate);
    mInitialized = true;
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
    gpio_config(&io_conf);
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
}

void UartTransport::deinitUart()
{
    if (mInitialized)
    {
        uart_driver_delete(mUartNumber);
        ESP_LOGI(spTag, "UART%d deinitialized.", mUartNumber);
        mInitialized = false;
    }
}

void UartTransport::sendUartMessage(const char *pLogTag, UartMessage &rMessage)
{
    uint8_t txBuffer[UART_PACKET_SIZE];
    serializeMessage(rMessage, txBuffer);

    ESP_LOGI(pLogTag, "Sending %s message (cmd=0x%04X)", pLogTag, rMessage.commandId);

    if (!sendData(txBuffer, UART_PACKET_SIZE))
    {
        ESP_LOGE(pLogTag, "Failed to send %s message", pLogTag);
    }
    else
    {
        ESP_LOGI(pLogTag, "%s message sent successfully", pLogTag);
    }
}

bool UartTransport::sendData(const uint8_t *pData, size_t len)
{
    if (!pData || len == 0 || !mInitialized)
    {
        ESP_LOGW(spTag, "Invalid send attempt.");
        return false;
    }

    if (gpio_get_level(PIN_ESP32_DATA_READY) == 1)
    {
        ESP_LOGW(spTag, "Raspberry Pi not ready to receive data.");
        return false;
    }

    ESP_LOGI(spTag, "Sending data of length %zu", len);
    int written = uart_write_bytes(mUartNumber, pData, len);

    ESP_LOGI(spTag, "Data sent, signaling Raspberry Pi.");
    ESP_ERROR_CHECK(gpio_set_level(PIN_ESP32_DATA_READY, 1));
    vTaskDelay(pdMS_TO_TICKS(kDataReadySignalHoldMs));
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
    return written == len;
}

void UartTransport::sendUartCommand(const char *pLogTag, uint32_t commandId)
{
    UartMessage msg{};
    msg.commandId = commandId;
    sendUartMessage(pLogTag, msg);
}

void IRAM_ATTR UartTransport::gpioIsrHandler(void *pArg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    auto *pSelf = static_cast<UartTransport *>(pArg);

    if (pSelf->mpTaskHandle)
    {
        vTaskNotifyGiveFromISR(pSelf->mpTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void UartTransport::handleUartRx()
{
    if (mInitialized)
    {
        int receivedDataLength = uart_read_bytes(mUartNumber, mTmpBuffer, TMP_BUFFER_SIZE, UART_PACKET_SIZE / portTICK_PERIOD_MS);

        if (receivedDataLength > 0)
        {
            mRxBuffer.pushBytes(mTmpBuffer, receivedDataLength);

            UartMessage msg;
            while (mRxBuffer.getNextMessage(msg))
            {
                if (mRxCallback)
                {
                    mLastRxTimeUs = esp_timer_get_time();
                    mRxCallback(msg);
                }
                else
                {
                    ESP_LOGW(spTag, "Failed to parse message");
                }
            }
        }
    }
    else
    {
        ESP_LOGW(spTag, "UART not initialized, cannot handle RX");
    }
}

void UartTransport::startHeartbeatMonitor(uint32_t timeoutMs,
                                   std::function<void()> onTimeout)
{
    mHeartbeatTimeoutMs = timeoutMs;
    mHeartbeatTimeoutCallback = onTimeout;
    mLastRxTimeUs = esp_timer_get_time();

    if (mpHeartbeatTaskHandle == nullptr)
    {
        xTaskCreate(
            [](void *arg)
            {
                UartTransport *pSelf = static_cast<UartTransport *>(arg);
                const TickType_t delay = pdMS_TO_TICKS(kHeartbeatCheckIntervalMs);

                while (true)
                {
                    vTaskDelay(delay);

                    uint64_t now = esp_timer_get_time();
                    uint64_t last = pSelf->mLastRxTimeUs;

                    if (last == 0)
                    {
                        continue;
                    }

                    uint64_t diff_ms = (now - last) / 1000;

                    if (diff_ms > pSelf->mHeartbeatTimeoutMs)
                    {
                        if (pSelf->mHeartbeatTimeoutCallback)
                            pSelf->mHeartbeatTimeoutCallback();

                        pSelf->mLastRxTimeUs = now;
                    }
                }
            },
            "heartbeat_task",
            kHeartbeatTaskStackSize,
            this,
            kHeartbeatTaskPriority,
            &mpHeartbeatTaskHandle);
    }
}

void UartTransport::stopHeartbeatMonitor()
{
    if (mpHeartbeatTaskHandle)
    {
        vTaskDelete(mpHeartbeatTaskHandle);
        mpHeartbeatTaskHandle = nullptr;
    }
}

void UartTransport::runUartRxTask()
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        handleUartRx();
    }
}

void UartTransport::setRxCallback(std::function<void(const UartMessage &)> callback)
{
    mRxCallback = callback;
}