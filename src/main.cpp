#include "main.h"
#include "input/input.hpp"
#include "esp_pm.h"


extern "C" void app_main(void)
{
    esp_pm_config_t pmConfig = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 40,
        .light_sleep_enable = true
    };
    esp_pm_configure(&pmConfig);

    vTaskDelay(pdMS_TO_TICKS(5000));  //this is really unfortunate , but we need to wait for the power to stabilise before we start doing anything

    controlSystem::ControlBoard board;
    board.init();

    // loop to  keep the app alive, do not put anything here as it will block the main thread and fuck everything up
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
