
#include "controlboard.hpp"
#include "powerLed.hpp"
#include "relay.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4



namespace controlSystem
{

    void ControlBoard::init()
    {

        // this command is only executed when the ESP32 is first started
        //  system witll default to Standby state
        indicators::PowerLed powerLed(PIN_APP_ACTIVE_LED, PIN_APP_STANDBY_LED);

        powerLed.setState(ControlBoardState::Standby);
        void SetdefaultRelays();

        // reset powerr supply relays to off

        while (1 == 1) // Infinite loop to keep the control board running
        {
            // Example: Initialize power LED
            indicators::PowerLed powerLed(PIN_APP_ACTIVE_LED, PIN_APP_STANDBY_LED);
            powerLed.setState(ControlBoardState::Standby);
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
            powerLed.setState(ControlBoardState::Active);

            vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
        }
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
    }

}