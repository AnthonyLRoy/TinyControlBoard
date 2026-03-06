#include "led_manager.hpp"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4
#define PIN_WORKING_STATUS_LED GPIO_NUM_48
#define STP_LEDS_BRIGHTNESS_LEVEL  GPIO_NUM_21
#define PIN_MONITOR_BRIGHTNESS GPIO_NUM_43

#define PIN_SPI_DATA GPIO_NUM_7
#define PIN_SPI_CLK GPIO_NUM_6 
#define PIN_SPI_LATCH GPIO_NUM_5
#define SPI_HOST spi_host_device_t::SPI2_HOST
namespace indicators {

    static ActiveLed sActiveLed(PIN_WORKING_STATUS_LED, LEDC_CHANNEL_0);
    static ActiveLed sButtonLeds(STP_LEDS_BRIGHTNESS_LEVEL, LEDC_CHANNEL_1);
    static PowerLed sPowerLed(PIN_APP_ACTIVE_LED, LEDC_CHANNEL_ON, PIN_APP_STANDBY_LED, LEDC_CHANNEL_STANDBY);
    static SpiLedDriver sSpiLedDriver(SPI_HOST, PIN_SPI_DATA, PIN_SPI_CLK, PIN_SPI_LATCH);
    static MonitorBrightnessController sMonitorBrightnessController(PIN_MONITOR_BRIGHTNESS, LEDC_CHANNEL_MONITOR_BRIGHTNESS);


    MonitorBrightnessController& getMonitorBrightnessController() {
        if (!sMonitorBrightnessController.mStarted)
        {
            sMonitorBrightnessController.init();
            sMonitorBrightnessController.mStarted = true;
        }
        return sMonitorBrightnessController;
    }
    
    ActiveLed& getActiveLed() {
        return sActiveLed;
    }

    PowerLed& getPowerLed() {
        if (!sPowerLed.mStarted)
        {
            sPowerLed.init();
            sPowerLed.mStarted = true;
        }
        return sPowerLed;
    }

    ActiveLed& getButtonLed() {
        return sButtonLeds;
    }
    SpiLedDriver& getSpiLedDriver() {
        if (!sSpiLedDriver.isStarted())
        {
            sSpiLedDriver.init();
        }
        return sSpiLedDriver;
    }
}