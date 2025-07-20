
#include "controlboard.hpp"
#include <driver/i2c.h>
#include "mcpHandler.hpp"
#include "freertos/FreeRTOS.h"

extern "C" void app_main()
{

    controlSystem::ControlBoard controlBoard;

    controlBoard.init();
    buttons::MCPInputHandler mcpHandler(0x20, I2C_NUM_0, GPIO_NUM_5);
    mcpHandler.begin();

    mcpHandler.setButtonCallback([](uint8_t pin, bool pressed)
                                 { ESP_LOGI("MCP", "Button %d %s", pin, pressed ? "pressed" : "released"); });

    mcpHandler.setReleaseCallback([](uint8_t pin, bool released)
                                  { ESP_LOGI("MCP", "Button %d released", pin); });

    mcpHandler.setRotaryCallback([](int movement)
                                 { ESP_LOGI("MCP", "Rotary movement: %d", movement); });

    while (true)
    {

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}
