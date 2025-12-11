#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"

namespace indicators
{
    class SpiLedDriver
    {

    public:
        SpiLedDriver(spi_host_device_t spiHost, gpio_num_t mosi_pin, gpio_num_t clk_pin, gpio_num_t latchPin);
        ~SpiLedDriver();

        bool started = false;
        void init();
        void setLed(uint16_t index, bool on);
        void update();

    private:
        static constexpr const char *TAG = "spiLedDriver";
        spi_device_handle_t spiHandle;
        spi_host_device_t host;
        gpio_num_t mosiPin;
        gpio_num_t clkPin;
        gpio_num_t latch;
        uint16_t ledState = 0;
        uint8_t txBuf[2];
    };
}
