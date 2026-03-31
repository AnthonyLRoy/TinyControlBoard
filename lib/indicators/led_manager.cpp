#include "led_manager.hpp"
#include "board/boardConfig.hpp"

#define SPI_HOST spi_host_device_t::SPI2_HOST
namespace indicators {

    static StatusLed sActivityStatusLed(board::indicators::kWorkingStatusLed, LEDC_CHANNEL_0);
    static StatusLed sButtonStatusLed(board::indicators::kButtonLedPwmPin,
                                      LEDC_CHANNEL_1,
                                      board::indicators::kButtonLedDefaultDuty,
                                      ControlBoardWorkingStatus::SolidIdle);
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
    
    StatusLed& getActivityStatusLed() {
        return sActivityStatusLed;
    }

    PowerLed& getPowerLed() {
        if (!sPowerLed.mStarted)
        {
            sPowerLed.init();
            sPowerLed.mStarted = true;
        }
        return sPowerLed;
    }

    StatusLed& getButtonStatusLed() {
        return sButtonStatusLed;
    }
    SpiLedDriver& getSpiLedDriver() {
        if (!sSpiLedDriver.isStarted())
        {
            sSpiLedDriver.init();
        }
        return sSpiLedDriver;
    }
}