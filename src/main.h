#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver\gpio.h"
#include "app/ControlBoard.hpp"
#include "indicators/ledManager.hpp"
#include "esp_timer.h"
#include "protocol/uartProtocol.hpp"
#include "transport/uart/serial.hpp"
class MainApp
{
public:
    void runAppMain(void);
    void run();
private:
    void performStartupTasks();

};