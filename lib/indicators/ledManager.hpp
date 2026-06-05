#pragma once

#include "indicators/statusLed.hpp"
#include "indicators/monitorBrightnessController.hpp"
#include "indicators/powerLed.hpp"
#include "hal/leds/spiLedDriver.hpp"
#include "indicators/BootDiagnosticLeds.hpp"

namespace indicators
{
    indicators::StatusLed &getActivityStatusLed();
    indicators::PowerLed &getPowerLed();
    indicators::StatusLed &getButtonStatusLed();
    indicators::SpiLedDriver &getSpiLedDriver();
    indicators::MonitorBrightnessController &getMonitorBrightnessController();
    indicators::BootDiagnosticLeds &getBootDiagnosticLeds();
}