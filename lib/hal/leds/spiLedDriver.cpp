#include "spiLedDriver.hpp"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

namespace indicators
{

    SpiLedDriver::SpiLedDriver(spi_host_device_t spiHost, gpio_num_t mosiPin, gpio_num_t clkPin, gpio_num_t latchPin)
        : m_host(spiHost), m_mosiPin(mosiPin),
          m_clkPin(clkPin), m_latchPin(latchPin)
    {
    }
    SpiLedDriver::~SpiLedDriver()
    {
    }
    bool SpiLedDriver::init()
    {
        ESP_LOGW(k_logTag, "Initializing LED Driver on SPI host %d with latch pin %d", m_host, m_latchPin);

        spi_bus_config_t buscfg = {};
            buscfg.mosi_io_num = m_mosiPin;
            buscfg.miso_io_num = -1;
            buscfg.sclk_io_num = m_clkPin;
            buscfg.quadwp_io_num = -1;
            buscfg.quadhd_io_num = -1;
            buscfg.max_transfer_sz = 4096;

        esp_err_t err = spi_bus_initialize(m_host, &buscfg, SPI_DMA_CH_AUTO);
        if (err != ESP_OK)
        {
            ESP_LOGE(k_logTag, "spi_bus_initialize failed: %d", err);
            return false;
        }

        spi_device_interface_config_t devcfg = {};
            devcfg.mode = 0;
            devcfg.clock_speed_hz = 1 * 1000 * 1000;
            devcfg.spics_io_num = -1;
            devcfg.queue_size = 1;

        err = spi_bus_add_device(m_host, &devcfg, &mp_spiHandle);
        if (err != ESP_OK)
        {
            ESP_LOGE(k_logTag, "spi_bus_add_device failed: %d", err);
            return false;
        }

        gpio_config_t latch_config = {};
            latch_config.pin_bit_mask = 1ULL << m_latchPin;
            latch_config.mode = GPIO_MODE_OUTPUT;
            latch_config.pull_up_en = GPIO_PULLUP_DISABLE;
            latch_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            latch_config.intr_type = GPIO_INTR_DISABLE;
            err = gpio_config(&latch_config);
        if (err != ESP_OK)
        {
            ESP_LOGE(k_logTag, "gpio_config failed: %d", err);
            return false;
        }

        gpio_set_level(m_latchPin, 0);
        m_started = true;
        return true;
    }

    void SpiLedDriver::setLed(uint8_t ledIndex, bool on)
    {
        if (!m_started)
        {
            ESP_LOGW(k_logTag, "setLed called before init");
            return;
        }
        if (ledIndex >= LED_COUNT)
        {
            ESP_LOGW(k_logTag, "setLed out of range: %u", ledIndex);
            return;
        }

        const uint16_t mask = static_cast<uint16_t>(1U << ledIndex);
        if (on)
        {
            m_ledBitState |= mask;
        }
        else
        {
            m_ledBitState &= static_cast<uint16_t>(~mask);
        }

        update();
    }

    void SpiLedDriver::setAllLeds(bool on)
    {
        if (!m_started)
        {
            ESP_LOGW(k_logTag, "setAllLeds called before init");
            return;
        }
        m_ledBitState = on ? 0xFFFFu : 0x0000u;
        update();
    }

    void SpiLedDriver::update()
    {
        if (!m_started)
        {
            ESP_LOGW(k_logTag, "update called before init");
            return;
        }
        m_txBuf[0] = (uint8_t)(m_ledBitState & 0xFF);
        m_txBuf[1] = (uint8_t)(m_ledBitState >> 8);

        spi_transaction_t transaction = {};
            transaction.length = 16;
            transaction.tx_buffer = m_txBuf;
        const esp_err_t err = spi_device_transmit(mp_spiHandle, &transaction);
        if (err != ESP_OK)
        {
            ESP_LOGE(k_logTag, "SPI transmit failed (err=0x%x); LED state may be stale", err);
            return;
        }

        esp_rom_delay_us(5);
        gpio_set_level(m_latchPin, 1);
        esp_rom_delay_us(20);
        gpio_set_level(m_latchPin, 0);
    }
}