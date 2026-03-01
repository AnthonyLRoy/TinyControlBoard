#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"

namespace indicators
{
    class SpiLedDriver
    {

    public:
        SpiLedDriver(spi_host_device_t spiHost, gpio_num_t mosiPin, gpio_num_t clkPin, gpio_num_t latchPin);
        ~SpiLedDriver();

        bool mStarted = false;
        void init();
        void setLed(uint8_t buttonId, bool on);
        void update();

    private:

        static constexpr const char *mspTag = "SpiLedDriver";
        spi_device_handle_t mpSpiHandle;
        spi_host_device_t mHost;
        gpio_num_t mMosiPin;
        gpio_num_t mClkPin;
        gpio_num_t mLatchPin;
        uint16_t mLedBitState = 0;
        uint8_t mTxBuf[2];
        void printU16Binary(uint16_t value);
    };
}
