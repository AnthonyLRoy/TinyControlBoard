
#include "controlboard.hpp"
#include "powerLed.hpp"
#include "activeLed.hpp"
#include "relay.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_Manager.hpp"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4



namespace controlSystem
{
    void ControlBoard::init()
    {
        // this command is only executed when the ESP32 is first started
        //  system witll default to Standby state

        indicators::getPowerLed().setState(ControlBoardState::Standby);
        SetRelaySystemPowerOnStatus();
    };

    void ControlBoard::SetRelaySystemPowerOnStatus()
    {
        // Initialize all relays with their respective GPIO pins
        relays::StandardRelay::init(PIN_RELAY_SCREEN);
        relays::StandardRelay::init(PIN_RELAY_DAC);
        relays::StandardRelay::init(PIN_RELAY_MAINS);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_2);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_3);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_4);

        // Set all relays to off state initially
        relays::StandardRelay::setRelayState(PIN_RELAY_SCREEN, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_DAC, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_MAINS, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_2, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_3, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_4, false);

        indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);
    }
}