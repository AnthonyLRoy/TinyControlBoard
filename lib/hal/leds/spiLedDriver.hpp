#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"

namespace indicators
{
    class SpiLedDriver
    {

    public:
        // Creates an SPI LED driver configured for its bus and latch GPIOs.
        SpiLedDriver(spi_host_device_t spiHost, gpio_num_t mosiPin, gpio_num_t clkPin, gpio_num_t latchPin);
        // Destroys the SPI driver wrapper; ESP-IDF manages the bus and device lifetime.
        ~SpiLedDriver();

        // Initializes the SPI bus, LED device, and output latch pin.
        bool init();
        // Reports whether the SPI LED hardware has been initialized successfully.
        bool isStarted() const { return m_started; }
        // Changes one LED in the cached bit pattern and sends the updated pattern to the driver.
        void setLed(uint8_t ledIndex, bool on);
        // Turns all LEDs on or off and sends the complete pattern to the driver.
        void setAllLeds(bool on);
        // Transmits the cached LED bit pattern and pulses the latch to apply it.
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
    };
}
