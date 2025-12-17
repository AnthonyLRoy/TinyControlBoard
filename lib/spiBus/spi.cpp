#include "spi.hpp"

static const char *TAG = "SPI";


//TODO  we can remove this is class as its been replaced with thw SpiLedDriver we are not going to use spi for anything else
namespace spibus
{

    SPI &SPI::instance(spi_host_device_t spi_host)
    {
        static SPI instance(spi_host);
        return instance;
    }

    SPI::SPI(spi_host_device_t spi_host) : host(spi_host) {}

    SPI::~SPI()
    {
        if (spi)
        {
            spi_bus_remove_device(spi);
            spi = nullptr;
        }
        spi_bus_free(host);
    }

    void SPI::init(gpio_num_t serial_data_out, gpio_num_t data_clock_out, int mhz)
    {
        serialDataOutPin = serial_data_out;
        serialDataClockPin = data_clock_out;

        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = serialDataOutPin;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = serialDataClockPin;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 4096;

        ESP_ERROR_CHECK(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO));

        spi_device_interface_config_t devcfg = {};
        devcfg.clock_speed_hz = mhz * 1000 * 1000;
        devcfg.mode = 0;
        devcfg.spics_io_num = -1;
        devcfg.queue_size = 1;

        ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &spi));
        ESP_LOGI(TAG, "SPI initialized on host %d at %d MHz", host, mhz);
    }

    void SPI::send(uint16_t data)
    {
        spi_transaction_t t = {};
        t.length = 16;
        t.tx_buffer = &data;

        ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
        ESP_LOGI(TAG, "Sent: 0x%04X", data);
    }

    void SPI::send_bulk(const uint16_t *data, size_t word_count)
    {
        spi_transaction_t t = {};
        t.length = word_count * 16;
        t.tx_buffer = data;

        ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
        ESP_LOGI(TAG, "Sent %u words", word_count);
    }

    void SPI::pulse_latch(gpio_num_t latch_pin)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << latch_pin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);

        gpio_set_level(latch_pin, 1);
        esp_rom_delay_us(10);
        gpio_set_level(latch_pin, 0);
        ESP_LOGI(TAG, "Latch pulse on GPIO %d", latch_pin);
    }

} // namespace spibus
