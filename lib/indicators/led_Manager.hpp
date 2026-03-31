#pragma once

#include "activeLed.hpp"
#include "monitorBrightnessController.hpp"
#include "powerLed.hpp"
#include "spiLedDriver.hpp"

namespace indicators
{
    indicators::StatusLed &getActiveLed();
    indicators::PowerLed &getPowerLed();
    indicators::StatusLed &getButtonLed();
    indicators::SpiLedDriver &getSpiLedDriver();
    indicators::MonitorBrightnessController &getMonitorBrightnessController();
}