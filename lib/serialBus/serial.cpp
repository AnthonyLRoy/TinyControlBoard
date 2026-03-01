#include "serial.hpp"
#include <cstring>
#include <esp_timer.h>

using namespace serialBus;

static const char *spTag = "SERIAL";

Serial &Serial::getInstance()
{
    static Serial sInstance;
    return sInstance;
}

Serial::Serial() : mUartNumber(UART_NUM_0), mInitialized(false) {}

Serial::~Serial()
{
    deinitUart();
}

bool Serial::initUart(uart_port_t uartNum,
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
        .source_clk = UART_SCLK_APB,
    };

    ESP_LOGI(spTag, "Configuring UART%d: %d baud, TX=%d, RX=%d", mUartNumber, baudRate, txPin, rxPin);

    // -------------------------
    // 1. Configure RPi Data Ready pin (input with interrupt)
    // -------------------------
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE; // Rising edge
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RPI_DATA_READY);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; // Depends on wiring
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    // -------------------------
    // 2. Create RX task BEFORE adding ISR
    // -------------------------
    if (!mInitialized)
    {
        xTaskCreate([](void *arg)
                    { static_cast<Serial *>(arg)->runUartRxTask(); }, "uart_rx_task", 4096, this, 10, &this->mpTaskHandle);
    }

    // -------------------------
    // 3. Install ISR service (only once globally)
    // -------------------------
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

    // -------------------------
    // 4. Attach ISR handler
    // -------------------------
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_RPI_DATA_READY, gpioIsrHandler, (void *)this));

    // -------------------------
    // 5. Initialize ESP32 Data Ready pin (output, signaling to RPi)
    // -------------------------
    gpio_config_t io_conf_out = {};
    io_conf_out.pin_bit_mask = (1ULL << PIN_ESP32_DATA_READY);
    io_conf_out.mode = GPIO_MODE_OUTPUT;
    io_conf_out.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf_out.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_out.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf_out);
    gpio_set_level(PIN_ESP32_DATA_READY, 0);

    // -------------------------
    // 6. UART driver setup
    // -------------------------
    ESP_ERROR_CHECK(uart_driver_install(mUartNumber, bufferSize * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(mUartNumber, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(mUartNumber, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(spTag, "UART%d initialized at %d baud.", mUartNumber, baudRate);
    mInitialized = true;

    // Optional: set RX callback
    // setRxCallback([this](const UartMessage &msg)
    //                 {
                              


    //                      ESP_LOGI(TAG, "Received message: cmd=0x%04X", msg.command_id);
    //                 });

    return true;
}

void Serial::initDataReadyPin()
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

void Serial::deinitUart()
{
    if (mInitialized)
    {
        uart_driver_delete(mUartNumber);
        ESP_LOGI(spTag, "UART%d deinitialized.", mUartNumber);
        mInitialized = false;
    }
}

void Serial::sendUartMessage(const char *pLogTag, UartMessage &rMessage)
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

bool Serial::sendData(const uint8_t *pData, size_t len)
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
    ESP_ERROR_CHECK(gpio_set_level(PIN_ESP32_DATA_READY, 1)); // Indicate data is ready
    vTaskDelay(pdMS_TO_TICKS(10));                            // Small delay
    gpio_set_level(PIN_ESP32_DATA_READY, 0);
    return written == len;
}

void Serial::sendUartCommand(const char *pLogTag, uint32_t commandId)
{
    UartMessage msg{};
    msg.commandId = commandId;
    sendUartMessage(pLogTag, msg);
}

void IRAM_ATTR Serial::gpioIsrHandler(void *pArg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    auto *pSelf = static_cast<Serial *>(pArg);

    if (pSelf->mpTaskHandle)
    {
        // Notify RX task from ISR
        vTaskNotifyGiveFromISR(pSelf->mpTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void Serial::handleUartRx()
{
    if (mInitialized)
    {
        int receivedDataLength = uart_read_bytes(mUartNumber, mTmpBuffer, TMP_BUFFER_SIZE, UART_PACKET_SIZE / portTICK_PERIOD_MS);

        if (receivedDataLength > 0)
        {
            // ESP_LOGI(TAG, "UART RX: Read %d bytes", len);
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

void Serial::startHeartbeatMonitor(uint32_t timeoutMs,
                                   std::function<void()> onTimeout)
{
    mHeartbeatTimeoutMs = timeoutMs;
    mHeartbeatTimeoutCallback = onTimeout;

    // Initialize last_rx_time_us to current time so timeout begins immediately
    mLastRxTimeUs = esp_timer_get_time();

    if (mpHeartbeatTaskHandle == nullptr)
    {
        xTaskCreate(
            [](void *arg)
            {
                Serial *pSelf = static_cast<Serial *>(arg);
                const TickType_t delay = pdMS_TO_TICKS(100);

                while (true)
                {
                    vTaskDelay(delay);

                    uint64_t now = esp_timer_get_time();
                    uint64_t last = pSelf->mLastRxTimeUs;

                    if (last == 0)
                    {
                        // No messages yet — do nothing
                        continue;
                    }

                    uint64_t diff_ms = (now - last) / 1000;

                    if (diff_ms > pSelf->mHeartbeatTimeoutMs)
                    {
                        // Trigger callback ONCE
                        if (pSelf->mHeartbeatTimeoutCallback)
                            pSelf->mHeartbeatTimeoutCallback();

                        // Reset timestamp so callback fires only once
                        pSelf->mLastRxTimeUs = now;
                    }
                }
            },
            "heartbeat_task",
            4096,
            this,
            5,
            &mpHeartbeatTaskHandle);
    }
}

void Serial::stopHeartbeatMonitor()
{
    if (mpHeartbeatTaskHandle)
    {
        vTaskDelete(mpHeartbeatTaskHandle);
        mpHeartbeatTaskHandle = nullptr;
    }
}

void Serial::runUartRxTask()
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        handleUartRx();
    }
}

void Serial::setRxCallback(std::function<void(const UartMessage &)> callback)
{
    mRxCallback = callback;
}
