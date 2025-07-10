
#include "controlboard.hpp"
#include "powerLed.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4

namespace controlSystem
{

    void ControlBoard::init()
    {
        while (1 == 1) // Infinite loop to keep the control board running
        {
            // Example: Initialize power LED
            indicators::PowerLed powerLed(PIN_APP_ACTIVE_LED, PIN_APP_STANDBY_LED);
            powerLed.setState(ControlBoardState::Standby);
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
            powerLed.setState(ControlBoardState::Active);

            vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
        }
    }

}