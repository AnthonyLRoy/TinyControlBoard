#include "mcpHandler.hpp"
#include "led_manager.hpp"
#include "esp_timer.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace buttons;
using namespace indicators;

static const char *TAG = "TEST";

extern "C" void app_main() {

    ESP_LOGI(TAG, "Starting Control Board Application");
    vTaskDelay(pdMS_TO_TICKS(8000)); // Delay to allow system to stabilize
    ESP_LOGI(TAG,"starting MCP23017 test");
    MCPInputHandler mcpHandler(0x20, I2C_NUM_0, GPIO_NUM_18);

    mcpHandler.setTimeout(10); // e.g., timeout = 1000 ms


    ESP_LOGI(TAG, "Starting test...");
    ESP_ERROR_CHECK(mcpHandler.begin());

    ESP_LOGI(TAG, "MCP23017 initialized successfully");
    mcpHandler.setTimeout(10); // Set timeout for I2C commands
    mcpHandler.scanner(); // Optional: scan I2C bus for devices
    mcpHandler.testConnection(0x20, 1000);
    // Set initial LED state to Idle
    getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);

    // Button press callback
    mcpHandler.setButtonCallback([](uint8_t pin, bool pressed) {
        ESP_LOGI(TAG, "Pin %u %s", pin, pressed ? "PRESSED" : "RELEASED");

        // Visual feedback: set LED to "doing work" if pressed
        if (pressed) {
            getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
        }
    });

    // Button release callback
    mcpHandler.setReleaseCallback([](uint8_t pin, bool released) {
        ESP_LOGI(TAG, "Pin %u RELEASED", pin);
        getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);
    });

    // Rotary encoder movement
    mcpHandler.setRotaryCallback([](int movement) {
        ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
        getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
    });

    // Optional idle/sleep LED indication if no activity
    while (true) {
        static int64_t lastActionTime = esp_timer_get_time();
        int64_t now = esp_timer_get_time();

        if ((now - lastActionTime) > 5000000) { // 5 seconds
            getActiveLed().SetStatus(ControlBoardWorkingStatus::sleeping);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
