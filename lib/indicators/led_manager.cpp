#include "led_manager.hpp"
#include "board/boardConfig.hpp"

#define SPI_HOST spi_host_device_t::SPI2_HOST
namespace indicators {

    static ActiveLed sActiveLed(board::indicators::kWorkingStatusLed, LEDC_CHANNEL_0);
    static ActiveLed sButtonLeds(board::indicators::kButtonLedBrightness, LEDC_CHANNEL_1);
    static PowerLed sPowerLed(board::indicators::kAppActiveLed, LEDC_CHANNEL_ON, board::indicators::kAppStandbyLed, LEDC_CHANNEL_STANDBY);
    static SpiLedDriver sSpiLedDriver(SPI_HOST, board::indicators::kSpiData, board::indicators::kSpiClock, board::indicators::kSpiLatch);
    static MonitorBrightnessController sMonitorBrightnessController(board::indicators::kMonitorBrightness, LEDC_CHANNEL_MONITOR_BRIGHTNESS);


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