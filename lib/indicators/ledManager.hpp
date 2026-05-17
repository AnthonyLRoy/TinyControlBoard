#pragma once

#include "statusLed.hpp"
#include "monitorBrightnessController.hpp"
#include "powerLed.hpp"
#include "spiLedDriver.hpp"
#include "SpiBootIndicator.hpp"

namespace indicators
{
    indicators::StatusLed &getActivityStatusLed();
    indicators::PowerLed &getPowerLed();
    indicators::StatusLed &getButtonStatusLed();
    indicators::SpiLedDriver &getSpiLedDriver();
    indicators::MonitorBrightnessController &getMonitorBrightnessController();
    indicators::SpiBootIndicator &getSpiBootIndicator();
}