#include "serial.hpp"

#include <cstring>
#include <esp_timer.h>

using namespace transport::uart;

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

    if (!m_handshake.configureRpiInputPin(PIN_RPI_DATA_READY))
    {
        return false;
    }

    if (!m_initialized.load(std::memory_order_acquire) && !m_rxPump.begin(m_uartNumber))
    {
        return false;
    }

    if (!m_handshake.installIsrHandler(PIN_RPI_DATA_READY, gpioIsrHandler, this))
    {
        return false;
    }

    if (!m_handshake.configureEsp32OutputPin(PIN_ESP32_DATA_READY))
    {
        return false;
    }

    esp_err_t ret = uart_driver_install(m_uartNumber, bufferSize * 2, 0, 0, nullptr, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to install UART driver (err=0x%x)", ret);
        return false;
    }
    ret = uart_param_config(m_uartNumber, &uart_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to configure UART parameters (err=0x%x)", ret);
        return false;
    }
    ret = uart_set_pin(m_uartNumber, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to set UART pins (err=0x%x)", ret);
        return false;
    }

    ESP_LOGI(k_logTag, "UART%d initialized at %d baud.", m_uartNumber, baudRate);
    m_initialized.store(true, std::memory_order_release);
    return true;
}

void UartTransport::deinitUart()
{
    stopHeartbeatMonitor();
    m_rxPump.stop();

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
    const uint8_t packetSize = serializeMessage(rMessage, txBuffer);

    ESP_LOGI(p_logTag, "Sending %s message (cmd=0x%04X)", p_logTag, rMessage.commandId);

    if (!sendData(txBuffer, packetSize))
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

    if (m_handshake.isPulseInProgress())
    {
        ESP_LOGW(k_logTag, "Raspberry Pi not ready to receive data");
        return false;
    }

    ESP_LOGI(k_logTag, "Sending data of length %zu", len);
    int written = uart_write_bytes(m_uartNumber, p_data, len);

    ESP_LOGI(k_logTag, "Data sent, signaling Raspberry Pi.");
    m_handshake.pulseDataReady();
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

    p_self->m_rxPump.notifyFromIsr(&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void UartTransport::startHeartbeatMonitor(uint32_t timeoutMs, std::function<void()> onTimeout)
{
    m_heartbeat.start(timeoutMs, std::move(onTimeout));
}

void UartTransport::stopHeartbeatMonitor()
{
    m_heartbeat.stop();
}

void UartTransport::setRxCallback(std::function<void(const UartMessage &)> callback)
{
    m_userRxCallback = std::move(callback);

    // Only the heartbeat monitor's last-RX timestamp advances alongside an actual
    // delivered message, matching the pre-refactor behavior of the combined class.
    m_rxPump.setMessageCallback([this](const UartMessage &rMsg)
                                 {
        m_heartbeat.notifyRx(esp_timer_get_time());
        if (m_userRxCallback)
        {
            m_userRxCallback(rMsg);
        } });
}
