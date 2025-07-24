#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver\gpio.h"
#include "mcpHandler.hpp"
#include "led_manager.hpp"
#include "esp_timer.h"

class main
{
public:
    void app_main(void);
    void run();
private:
    void PerformStartupTasks();

};