#include "spiLedDriver.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

namespace indicators
{

        SpiLedDriver::SpiLedDriver(spi_host_device_t spiHost, gpio_num_t mosiPin, gpio_num_t clkPin, gpio_num_t latchPin)
                : mHost(spiHost), mMosiPin(mosiPin),
                    mClkPin(clkPin), mLatchPin(latchPin)
    {
    }
    SpiLedDriver::~SpiLedDriver()
    {
    }
    void SpiLedDriver::init()
    {
        ESP_LOGW(mspTag, "Initializing LED Driver on SPI host %d with latch pin %d", mHost, mLatchPin);

        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = mMosiPin;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = mClkPin;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 4096;

        spi_bus_initialize(mHost, &buscfg, SPI_DMA_CH_AUTO);

        spi_device_interface_config_t devcfg = {};
        devcfg.mode = 0;
        devcfg.clock_speed_hz = 1 * 1000 * 1000;
        devcfg.spics_io_num = -1;
        devcfg.queue_size = 1;

        spi_bus_add_device(mHost, &devcfg, &mpSpiHandle);

        gpio_config_t latch_config = {};
        latch_config.pin_bit_mask = 1ULL << mLatchPin;
        latch_config.mode = GPIO_MODE_OUTPUT;
        latch_config.pull_up_en = GPIO_PULLUP_DISABLE;
        latch_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        latch_config.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&latch_config);

        gpio_set_level(mLatchPin, 0);
    }

    void SpiLedDriver::setLed(uint8_t buttonId, bool on)
    {
        if (on)
        {
            mLedBitState |= (1 << buttonId);
        }
        else
        {

            mLedBitState &= ~(1 << buttonId);
        }

        update();
    }

    void SpiLedDriver::update()
    {
        mTxBuf[0] = (uint8_t)(mLedBitState & 0xFF);
        mTxBuf[1] = (uint8_t)(mLedBitState >> 8);

        spi_transaction_t transaction = {};
        transaction.length = 16;
        transaction.tx_buffer = mTxBuf;
        ESP_LOGI(mspTag, "Sending SPI_MESSAGE: 0x%04X", mLedBitState);
        printU16Binary(mLedBitState);
        ESP_ERROR_CHECK(spi_device_transmit(mpSpiHandle, &transaction));

        esp_rom_delay_us(5);
        gpio_set_level(mLatchPin, 1);
        esp_rom_delay_us(20);
        gpio_set_level(mLatchPin, 0);
    }

    void SpiLedDriver::printU16Binary(uint16_t value)
    {
        for (int i = 15; i >= 0; --i)
            putchar((value & (1 << i)) ? '1' : '0');
        putchar('\n');
    }
}