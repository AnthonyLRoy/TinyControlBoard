

#include "spiLedDriver.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

namespace led
{

#define MOSI_PIN GPIO_NUM_7
#define SCLK_PIN GPIO_NUM_6

    spiLedDriver::spiLedDriver(spi_host_device_t spiHost, gpio_num_t latchPin)
        : host(spiHost), latch(latchPin) {}

    void spiLedDriver::begin()
    {

        ESP_LOGW("LEDDriver", "Initializing LED Driver on SPI host %d with latch pin %d", host, latch);
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = MOSI_PIN;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = SCLK_PIN;
        buscfg.max_transfer_sz = 2;

        spi_bus_initialize(host, &buscfg, SPI_DMA_DISABLED);

        spi_device_interface_config_t devcfg = {};
        devcfg.mode = 0;
        devcfg.clock_speed_hz = 1 * 1000 * 1000;
        devcfg.spics_io_num = -1;
        devcfg.queue_size = 1;

        spi_bus_add_device(host, &devcfg, &spiHandle);

        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = 1ULL << latch;
        io_conf.mode = GPIO_MODE_OUTPUT;

        gpio_config(&io_conf);
    }

    void spiLedDriver::setLed(uint8_t index, bool on)
    {
        if (on)
        {
            ledState |= (1 << index);
        }
        else
        {
            ledState &= ~(1 << index);
        }
    }

    void spiLedDriver::update()
    {
        uint8_t txBuf[2] = {(uint8_t)(ledState >> 8), (uint8_t)(ledState & 0xFF)};

        spi_transaction_t transaction = {};
        transaction.length = 16;
        transaction.tx_buffer = txBuf;
        spi_device_transmit(spiHandle, &transaction);

        gpio_set_level(latch, 0);
        esp_rom_delay_us(1);
        gpio_set_level(latch, 1);
    }
}