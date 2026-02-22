#pragma once
#include "activeLed.hpp"
#include "powerLed.hpp"
#include "spiLedDriver.hpp" 
#include "monitorBrightnessController.hpp"

namespace indicators
{
    indicators::ActiveLed& getActiveLed();
    indicators::PowerLed& getPowerLed();
    indicators::ActiveLed& getButtonLed();
    indicators::SpiLedDriver& getSpiLedDriver();
    indicators::MonitorBrightnessController& getMonitorBrightnessController();

}