#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <cstring>

namespace spibus {

class Spi {
public:
    static Spi& getInstance(spi_host_device_t spiHost = SPI2_HOST);

    void init(gpio_num_t serialDataOut, gpio_num_t dataClockOut, int mhz = 1);
    void send(uint16_t data);
    void sendBulk(const uint16_t *pData, size_t wordCount);
    void pulseLatch(gpio_num_t latchPin);

private:
    // Singleton pattern
    Spi(spi_host_device_t spiHost);
    ~Spi();

    // Delete copy and move
    Spi(const Spi&) = delete;
    Spi& operator=(const Spi&) = delete;
    Spi(Spi&&) = delete;
    Spi& operator=(Spi&&) = delete;

    spi_host_device_t mHost;
    spi_device_handle_t mpSpi = nullptr;
    gpio_num_t mSerialDataOutPin;
    gpio_num_t mSerialDataClockPin;
};

} // namespace spibus
