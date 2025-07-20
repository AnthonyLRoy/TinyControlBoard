#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"

namespace led
{

    class LEDDriver
    {
    public:
        LEDDriver(spi_host_device_t spiHost, gpio_num_t latchPin);
        void begin();
        void setLed(uint8_t index, bool on);
        void update();

    private:
        spi_device_handle_t spiHandle;
        spi_host_device_t host;
        gpio_num_t latch;
        uint16_t ledState = 0;
    };

}
