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
        bool isStarted() const { return m_started; }
        void setLed(uint8_t ledIndex, bool on);
        void setAllLeds(bool on);
        void update();

    private:

        static constexpr const char *k_logTag = "SPI_Led_Driver  ";
        static constexpr uint8_t LED_COUNT = 16;
        spi_device_handle_t mp_spiHandle;
        spi_host_device_t m_host;
        gpio_num_t m_mosiPin;
        gpio_num_t m_clkPin;
        gpio_num_t m_latchPin;
        uint16_t m_ledBitState = 0;
        uint8_t m_txBuf[2];
        bool m_started = false;
        void printU16Binary(uint16_t value);
    };
}
