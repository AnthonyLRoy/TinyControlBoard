#pragma once

#include "activeLed.hpp"
#include "monitorBrightnessController.hpp"
#include "powerLed.hpp"
#include "spiLedDriver.hpp"

namespace indicators
{
    indicators::ActiveLed &getActiveLed();
    indicators::PowerLed &getPowerLed();
    indicators::ActiveLed &getButtonLed();
    indicators::SpiLedDriver &getSpiLedDriver();
    indicators::MonitorBrightnessController &getMonitorBrightnessController();
}