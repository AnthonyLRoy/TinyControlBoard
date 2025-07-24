
#include "main.h"
#include "ControlBoard.hpp"

using namespace buttons;
using namespace indicators;
using namespace controlSystem;


#define GPPIO_NUM_SCDL GPIO_NUM_15
#define GPIO_NUM_SDA GPIO_NUM_16    
#define GPIO_NUM_INTERRUPT GPIO_NUM_18
#define MCP_ADDRESS 0x20 // Default I2C address for MCP23017

static const char *TAG = "CONTROL_BOARD";

ControlBoard controlBoardInstance = ControlBoard();
MCPInputHandler mcpHandler(MCP_ADDRESS, I2C_NUM_0); // MCP23017 handler

extern "C" void app_main(void)
{

    static main myApp;
    nvs_flash_init();
    myApp.run();

}

void main::run()
{
    PerformStartupTasks();

}


void main::PerformStartupTasks()
{

    vTaskDelay(pdMS_TO_TICKS(5000)); // Delay to allow system to stabilize
    
    ESP_LOGI(TAG, "Starting Control Board Application");
    // Initialize the control board
    controlBoardInstance.init(); // Initialize control board
    ESP_LOGI(TAG, "Control Board Initialized");
    ESP_LOGI(TAG, "Initialising  MCP23017");
    mcpHandler.setTimeout(10); // Set timeout for I2C commands
    ESP_ERROR_CHECK(mcpHandler.begin(GPIO_NUM_SDA, GPPIO_NUM_SCDL,GPIO_NUM_INTERRUPT)); // Initialize MCP23017 with SDA and SCL pins
    // Optional: scan I2C bus for devices    
    mcpHandler.scanner();

}


// public ot()
// {
    
//     controlBoardInstance.init(); // Initialize control board
//     ESP_LOGI(TAG, "Starting Control Board Application");
//     vTaskDelay(pdMS_TO_TICKS(8000)); // Delay to allow system to stabilize
//     ESP_LOGI(TAG, "starting MCP23017 test");
//     MCPInputHandler mcpHandler(0x20, I2C_NUM_0, GPIO_NUM_18);

//     mcpHandler.setTimeout(10); // e.g., timeout = 1000 ms

//     ESP_LOGI(TAG, "Starting test...");
//     ESP_ERROR_CHECK(mcpHandler.begin());

//     ESP_LOGI(TAG, "MCP23017 initialized successfully");
//     mcpHandler.setTimeout(10); // Set timeout for I2C commands
//     mcpHandler.scanner();      // Optional: scan I2C bus for devices
//     mcpHandler.testConnection(0x20, 1000);
//     // Set initial LED state to Idle
//     getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);

//     // Button press callback
//     mcpHandler.setButtonCallback([](uint8_t pin, bool pressed)
//                                  {
//         ESP_LOGI(TAG, "Pin %u %s", pin, pressed ? "PRESSED" : "RELEASED");

//         // Visual feedback: set LED to "doing work" if pressed
//         if (pressed) {
//             getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
//         } });

//     // Button release callback
//     mcpHandler.setReleaseCallback([](uint8_t pin, bool released)
//                                   {
//         ESP_LOGI(TAG, "Pin %u RELEASED", pin);
//         getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle); });

//     // Rotary encoder movement
//     mcpHandler.setRotaryCallback([](int movement)
//                                  {
//         ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
//         getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork); });

//     // Optional idle/sleep LED indication if no activity
//     while (true)
//     {
//         static int64_t lastActionTime = esp_timer_get_time();
//         int64_t now = esp_timer_get_time();

//         if ((now - lastActionTime) > 5000000)
//         { // 5 seconds
//             getActiveLed().SetStatus(ControlBoardWorkingStatus::sleeping);
//         }

//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }
