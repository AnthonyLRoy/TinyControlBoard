#include "main.h"
#include "input/input.hpp"
#include "esp_pm.h"

#define DELAY_STARTUP_TIME_MS 5000
#define MAX_CLOCK_FREQ_MHZ 240
#define MIN_CLOCK_FREQ_MHZ 40

extern "C" void app_main(void)
{

    // does not work :(  does not save any power at all 
    esp_pm_config_t pmConfig = {
        .max_freq_mhz = MAX_CLOCK_FREQ_MHZ,
        .min_freq_mhz = MIN_CLOCK_FREQ_MHZ,
        .light_sleep_enable = true
    };
    esp_pm_configure(&pmConfig);

    vTaskDelay(pdMS_TO_TICKS(DELAY_STARTUP_TIME_MS));  //this is really unfortunate , but we need to wait for the power to stabilise before we start doing anything

    controlSystem::ControlBoard board;
    while (!board.init())
    {
        ESP_LOGE("main", "ControlBoard init failed; retrying in 1s...");
        board.deinit();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Keep the app_main task alive; all work is done in FreeRTOS tasks.
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
