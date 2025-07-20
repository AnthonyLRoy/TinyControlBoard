

#include "LEDDriver.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

namespace led
{

#define MOSI_PIN GPIO_NUM_20
#define SCLK_PIN GPIO_NUM_18

    LEDDriver::LEDDriver(spi_host_device_t spiHost, gpio_num_t latchPin)
        : host(spiHost), latch(latchPin) {}

    void LEDDriver::begin()
    {
        spi_bus_config_t buscfg = {
            .mosi_io_num = MOSI_PIN,
            .miso_io_num = -1,
            .sclk_io_num = SCLK_PIN,
            .max_transfer_sz = 2,
        };
        
        spi_bus_initialize(host, &buscfg, SPI_DMA_DISABLED);

        spi_device_interface_config_t devcfg = {
            .mode= 0,
            .clock_speed_hz = 1 * 1000 * 1000,
            .spics_io_num = -1,
            .queue_size = 1,
        };

        spi_bus_add_device(host, &devcfg, &spiHandle);

        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << latch,
            .mode = GPIO_MODE_OUTPUT,
        };
        gpio_config(&io_conf);
    }

    void LEDDriver::setLed(uint8_t index, bool on)
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

    void LEDDriver::update()
    {
        uint8_t txBuf[2] = {(uint8_t)(ledState >> 8), (uint8_t)(ledState & 0xFF)};

        spi_transaction_t t = {
            .length = 16,
            .tx_buffer = txBuf};
        spi_device_transmit(spiHandle, &t);

        gpio_set_level(latch, 0);
        esp_rom_delay_us(1);
        gpio_set_level(latch, 1);
    }

}