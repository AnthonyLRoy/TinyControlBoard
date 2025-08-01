#pragma once
#include "activeLed.hpp"
#include "powerLed.hpp"

namespace indicators
{
    indicators::ActiveLed& getActiveLed();
    indicators::PowerLed& getPowerLed();
    indicators::ActiveLed& getButtonLed();

}