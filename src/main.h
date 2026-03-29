#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver\gpio.h"
#include "mcpInputHandler.hpp"
#include "led_manager.hpp"
#include "esp_timer.h"
#include "serial.hpp"
#include "protocol/uartProtocol.hpp"
class MainApp
{
public:
    void runAppMain(void);
    void run();
private:
    void performStartupTasks();

};