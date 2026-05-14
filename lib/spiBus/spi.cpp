#include "spi.hpp"

static const char *spTag = "SPI             ";


//TODO  we can remove this is class as its been replaced with thw SpiLedDriver we are not going to use spi for anything else
namespace spibus
{

    Spi &Spi::getInstance(spi_host_device_t spiHost)
    {
        static Spi sInstance(spiHost);
        return sInstance;
    }

    Spi::Spi(spi_host_device_t spiHost) : mHost(spiHost) {}

    Spi::~Spi()
    {
        if (mpSpi)
        {
            spi_bus_remove_device(mpSpi);
            mpSpi = nullptr;
        }
        spi_bus_free(mHost);
    }

    void Spi::init(gpio_num_t serialDataOut, gpio_num_t dataClockOut, int mhz)
    {
        mSerialDataOutPin = serialDataOut;
        mSerialDataClockPin = dataClockOut;

        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = mSerialDataOutPin;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = mSerialDataClockPin;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 4096;

        ESP_ERROR_CHECK(spi_bus_initialize(mHost, &buscfg, SPI_DMA_CH_AUTO));

        spi_device_interface_config_t devcfg = {};
        devcfg.clock_speed_hz = mhz * 1000 * 1000;
        devcfg.mode = 0;
        devcfg.spics_io_num = -1;
        devcfg.queue_size = 1;

        ESP_ERROR_CHECK(spi_bus_add_device(mHost, &devcfg, &mpSpi));
        ESP_LOGI(spTag, "SPI initialized on host %d at %d MHz", mHost, mhz);
    }

    void Spi::send(uint16_t data)
    {
        spi_transaction_t t = {};
        t.length = 16;
        t.tx_buffer = &data;

        ESP_ERROR_CHECK(spi_device_transmit(mpSpi, &t));
        ESP_LOGI(spTag, "Sent: 0x%04X", data);
    }

    void Spi::sendBulk(const uint16_t *pData, size_t wordCount)
    {
        spi_transaction_t t = {};
        t.length = wordCount * 16;
        t.tx_buffer = pData;

        ESP_ERROR_CHECK(spi_device_transmit(mpSpi, &t));
        ESP_LOGI(spTag, "Sent %u words", wordCount);
    }

    void Spi::pulseLatch(gpio_num_t latchPin)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << latchPin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);

        gpio_set_level(latchPin, 1);
        esp_rom_delay_us(10);
        gpio_set_level(latchPin, 0);
        ESP_LOGI(spTag, "Latch pulse on GPIO %d", latchPin);
    }

} // namespace spibus
