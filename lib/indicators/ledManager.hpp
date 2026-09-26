#pragma once

#include "indicators/statusLed.hpp"
#include "indicators/monitorBrightnessController.hpp"
#include "indicators/powerLed.hpp"
#include "hal/leds/spiLedDriver.hpp"
#include "indicators/BootDiagnosticLeds.hpp"

namespace indicators
{
    // Returns the activity-status LED controller.
    indicators::StatusLed &getActivityStatusLed();
    // Returns the lazily initialized power LED controller.
    indicators::PowerLed &getPowerLed();
    // Returns the button-status LED controller.
    indicators::StatusLed &getButtonStatusLed();
    // Returns the lazily initialized SPI button LED driver.
    indicators::SpiLedDriver &getSpiLedDriver();
    // Returns the lazily initialized monitor brightness controller.
    indicators::MonitorBrightnessController &getMonitorBrightnessController();
    // Returns the boot diagnostic LED service.
    indicators::BootDiagnosticLeds &getBootDiagnosticLeds();
}