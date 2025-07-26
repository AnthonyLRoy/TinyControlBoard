#pragma once

#include <cstdint>
#include <functional>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_Check.h"
#include <esp_log.h>


// registers
#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14


#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

#define MCP23017_INT_ERR 255



namespace buttons
{

    class MCPInputHandler
    {
    public:

    TaskHandle_t interruptTaskHandle = nullptr;
        MCPInputHandler(uint8_t address, i2c_port_t port);


        void interruptTaskLoop();

        // Must be called before any other method
        esp_err_t begin(gpio_num_t sda, gpio_num_t scl, gpio_num_t intPin);

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

        void I2CEnable(bool enable);

        uint8_t readRegister(uint8_t reg);

        void dumpRegisters();

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
