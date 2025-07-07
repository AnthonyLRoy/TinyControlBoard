
#include "controlboard.hpp"
#include "powerLed.hpp"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4

namespace controlSystem
{

    void ControlBoard::init()
    {
        // Example: Initialize power LED
        indicators::PowerLed powerLed(PIN_APP_ACTIVE_LED, PIN_APP_STANDBY_LED);
        powerLed.setState(ControlBoardState::Standby);
    }

}