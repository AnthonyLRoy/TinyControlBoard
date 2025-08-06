#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <cstring>

namespace spibus {

class SPI {
public:
    static SPI& instance(spi_host_device_t spi_host = SPI2_HOST);

    void init(gpio_num_t serial_data_out, gpio_num_t data_clock_out, int mhz = 1);
    void send(uint16_t data);
    void send_bulk(const uint16_t* data, size_t word_count);
    void pulse_latch(gpio_num_t latch_pin);

private:
    // Singleton pattern
    SPI(spi_host_device_t spi_host);
    ~SPI();

    // Delete copy and move
    SPI(const SPI&) = delete;
    SPI& operator=(const SPI&) = delete;
    SPI(SPI&&) = delete;
    SPI& operator=(SPI&&) = delete;

    spi_host_device_t host;
    spi_device_handle_t spi = nullptr;
    gpio_num_t sdo;
    gpio_num_t clk;
};

} // namespace spibus
