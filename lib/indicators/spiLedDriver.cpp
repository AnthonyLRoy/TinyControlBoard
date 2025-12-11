#include "spiLedDriver.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

namespace indicators
{
    static const char *TAG = "SpiLedDriver";

    SpiLedDriver::SpiLedDriver(spi_host_device_t spi_host, gpio_num_t mosi_pin, gpio_num_t clk_pin, gpio_num_t latchPin)
        : host(spi_host), mosiPin(mosi_pin),
          clkPin(clk_pin), latch(latchPin)
    {
    }
    SpiLedDriver::~SpiLedDriver()
    {
    }
    void SpiLedDriver::init()
    {
        ESP_LOGW("TAG", "Initializing LED Driver on SPI host %d with latch pin %d", host, latch);

        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = mosiPin;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = clkPin;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 4096;

        spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);

        spi_device_interface_config_t devcfg = {};
        devcfg.mode = 0;
        devcfg.clock_speed_hz = 1 * 1000 * 1000;
        devcfg.spics_io_num = -1;
        devcfg.queue_size = 1;

        spi_bus_add_device(host, &devcfg, &spiHandle);

        gpio_config_t latch_config = {};
        latch_config.pin_bit_mask = 1ULL << latch;
        latch_config.mode = GPIO_MODE_OUTPUT;
        latch_config.pull_up_en = GPIO_PULLUP_DISABLE;
        latch_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        latch_config.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&latch_config);

        gpio_set_level(latch,0);
    }

    void SpiLedDriver::setLed(uint16_t bitMask, bool on)
    {
        if (on)
        {
            ledState = 0xffff;
        }
        else
        {
            
            ledState &= 0x0000;
        }

        update();
    }

    void SpiLedDriver::update()
    {
        txBuf[0] = (uint8_t)(ledState & 0xFF);
        txBuf[1] = (uint8_t)(ledState >> 8);

        spi_transaction_t transaction = {};
        transaction.length = 16;
        transaction.tx_buffer = txBuf;
        ESP_LOGI(TAG, "Sending SPI_MESSAGE: 0x%04X", ledState);
        ESP_ERROR_CHECK(spi_device_transmit(spiHandle, &transaction));

        esp_rom_delay_us(5);
        gpio_set_level(latch, 1);
        esp_rom_delay_us(20);
        gpio_set_level(latch, 0);
    }
}