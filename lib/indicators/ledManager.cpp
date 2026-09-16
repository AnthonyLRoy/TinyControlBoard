#include "ledManager.hpp"
#include "board/boardConfig.hpp"

#define SPI_HOST spi_host_device_t::SPI2_HOST
namespace indicators {
    // Static instances of various LED drivers and controllers used throughout the system.
    static StatusLed s_activityStatusLed(board::indicators::k_workingStatusLed, LEDC_CHANNEL_0);
    static StatusLed s_buttonStatusLed(board::indicators::k_buttonLedPwmPin,
                                      LEDC_CHANNEL_1,
                                      board::indicators::k_buttonLedDefaultDuty,
                                      ControlBoardWorkingStatus::SolidIdle);
    static PowerLed s_powerLed(board::indicators::k_appActiveLed, LEDC_CHANNEL_ON, board::indicators::k_appStandbyLed, LEDC_CHANNEL_STANDBY);
    static SpiLedDriver s_spiLedDriver(SPI_HOST, board::indicators::k_spiData, board::indicators::k_spiClock, board::indicators::k_spiLatch);
    static MonitorBrightnessController s_monitorBrightnessController(board::indicators::k_monitorBrightness, LEDC_CHANNEL_MONITOR_BRIGHTNESS);
    static BootDiagnosticLeds s_bootDiagnosticLeds;


    MonitorBrightnessController& getMonitorBrightnessController() {
        if (!s_monitorBrightnessController.m_started)
        {
            s_monitorBrightnessController.init();
            s_monitorBrightnessController.m_started = true;
        }
        return s_monitorBrightnessController;
    }
    
    StatusLed& getActivityStatusLed() {
        return s_activityStatusLed;
    }

    PowerLed& getPowerLed() {
        if (!s_powerLed.m_started)
        {
            s_powerLed.init();
            s_powerLed.m_started = true;
        }
        return s_powerLed;
    }

    StatusLed& getButtonStatusLed() {
        return s_buttonStatusLed;
    }
    SpiLedDriver& getSpiLedDriver() {
        if (!s_spiLedDriver.isStarted())
        {
            s_spiLedDriver.init();
        }
        return s_spiLedDriver;
    }

    BootDiagnosticLeds& getBootDiagnosticLeds() {
        return s_bootDiagnosticLeds;
    }
}