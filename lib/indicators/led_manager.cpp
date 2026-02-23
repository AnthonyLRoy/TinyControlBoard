#include "led_manager.hpp"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4
#define PIN_WORKING_STATUS_LED GPIO_NUM_48
#define STP_LEDS_BRIGHTNESS_LEVEL  GPIO_NUM_21

#define PIN_SPI_DATA GPIO_NUM_7
#define PIN_SPI_CLK GPIO_NUM_6 
#define PIN_SPI_LATCH GPIO_NUM_5
#define SPI_HOST spi_host_device_t::SPI2_HOST
namespace indicators {

    static ActiveLed activeLed(PIN_WORKING_STATUS_LED,LEDC_CHANNEL_0);
    static ActiveLed buttonLeds(STP_LEDS_BRIGHTNESS_LEVEL,LEDC_CHANNEL_1);
    static PowerLed powerLed(PIN_APP_ACTIVE_LED,LEDC_CHANNEL_ON, PIN_APP_STANDBY_LED,LEDC_CHANNEL_STANDBY);
    static SpiLedDriver spiLedDriver(SPI_HOST,PIN_SPI_DATA,PIN_SPI_CLK,PIN_SPI_LATCH);
    static MonitorBrightnessController monitorBrightnessController(GPIO_NUM_1,LEDC_CHANNEL_MONITOR_BRIGHTNESS);


    MonitorBrightnessController& getMonitorBrightnessController() {
        if (!monitorBrightnessController.started)
        {
            monitorBrightnessController.init();
            monitorBrightnessController.started = true;
        }
        return monitorBrightnessController;
    }
    
    ActiveLed& getActiveLed() {
        return activeLed;
    }

    PowerLed& getPowerLed() {
        if (!powerLed.started)
        {
            powerLed.init();
            powerLed.started = true;
        }
        return powerLed;
    }

    ActiveLed& getButtonLed() {
        return buttonLeds;
    }
    SpiLedDriver& getSpiLedDriver() {
               if (!spiLedDriver.started)
        {
            spiLedDriver.init();
            spiLedDriver.started = true;
        }
        return spiLedDriver;
    }
}