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

        bool init();
        bool isStarted() const { return mStarted; }
        void setLed(uint8_t ledIndex, bool on);
        void setAllLeds(bool on);
        void update();

    private:

        static constexpr const char *kLogTag = "SPI_Led_Driver";
        static constexpr uint8_t LED_COUNT = 16;
        spi_device_handle_t mpSpiHandle;
        spi_host_device_t mHost;
        gpio_num_t mMosiPin;
        gpio_num_t mClkPin;
        gpio_num_t mLatchPin;
        uint16_t mLedBitState = 0;
        uint8_t mTxBuf[2];
        bool mStarted = false;
        void printU16Binary(uint16_t value);
    };
}
