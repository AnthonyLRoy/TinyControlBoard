#include "main.h"
#include "ControlBoard.hpp"
#include "spi.hpp"
#include "buttonActions.hpp"


extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(5000));

    controlSystem::ControlBoard board;
    board.init();

    // Optionally, your main loop or FreeRTOS tasks go here.
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
