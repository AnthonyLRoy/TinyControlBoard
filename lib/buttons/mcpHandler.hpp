#pragma once

#include <freertos/FreeRTOS.h>
#include <functional>
#include <freertos/timers.h>
#include <driver/i2c.h>
#include <map>
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/task.h"


namespace buttons
{
    class MCPInputHandler
    {
    public:
        MCPInputHandler(uint8_t address, i2c_port_t port, gpio_num_t intPin);
        void begin();
        void setButtonCallback(std::function<void(uint8_t, bool)> cb);
        void setReleaseCallback(std::function<void(uint8_t, bool)> cb);
        void setRotaryCallback(std::function<void(int)> cb);

    private:
        void handleInterrupt();
        uint16_t readGPIO16();
        static void timerCallback(TimerHandle_t xTimer);

        uint8_t i2cAddr;
        i2c_port_t i2cPort;
        gpio_num_t interruptPin;

        std::function<void(uint8_t, bool)> buttonCallback;
        std::function<void(uint8_t, bool)> releaseCallback;
        std::function<void(int)> rotaryCallback;

        uint16_t prevState = 0xFFFF;
        std::map<uint8_t, TimerHandle_t> timers;
        uint8_t rotaryLast = 0;
    };

}
