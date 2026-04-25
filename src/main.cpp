#include "main.h"
#include "spi.hpp"
#include "input/input.hpp"


extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(5000));  //this is really unfortunate , but we need to wait for the power to stabilise before we start doing anything

    controlSystem::ControlBoard board;
    board.init();

    // loop to  keep the app alive, do not put anything here 
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
