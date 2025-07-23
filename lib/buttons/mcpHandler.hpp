#pragma once

#include <cstdint>
#include <functional>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_Check.h"

namespace buttons {

class MCPInputHandler {
public:
    MCPInputHandler(uint8_t address, i2c_port_t port, gpio_num_t intPin);

    // Must be called before any other method
    esp_err_t begin();

    // Optional: scans I2C bus and logs found devices
    void scanner();

    // Optional: checks if given device is reachable
    esp_err_t testConnection(uint8_t devAddr, int32_t timeout = -1);

    // Sets time to wait for I2C commands
    void setTimeout(uint32_t ms);

    // Button press callback (pin, pressed)
    void setButtonCallback(std::function<void(uint8_t, bool)> cb);

    // Button release callback (pin, released)
    void setReleaseCallback(std::function<void(uint8_t, bool)> cb);

    // Rotary encoder callback (movement -1, 0, 1)
    void setRotaryCallback(std::function<void(int)> cb);

private:
    uint8_t i2cAddr;
    i2c_port_t i2cPort;
    gpio_num_t interruptPin;

    uint16_t prevState;
    uint8_t rotaryLast;
    TickType_t ticksToWait;

    TimerHandle_t timers[16];

    std::function<void(uint8_t, bool)> buttonCallback;
    std::function<void(uint8_t, bool)> releaseCallback;
    std::function<void(int)> rotaryCallback;

    // Internal handlers
    void handleInterrupt();
    void decodeRotary(uint16_t state);
    static void timerCallback(TimerHandle_t xTimer);

    // I2C helpers
    uint16_t readGPIO16();
    void writeRegister(uint8_t reg, uint8_t val);
    void writeRegisterPair(uint8_t baseReg, uint8_t a, uint8_t b);
};

} // namespace buttons
