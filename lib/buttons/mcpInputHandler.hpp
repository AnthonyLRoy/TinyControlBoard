#pragma once

#include "project_config.hpp"
#include <cstdint>
#include <functional>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_Check.h"
#include <esp_log.h>




// MCP23018 register addresses
constexpr uint8_t MCP_IODIRA   = 0x00;
constexpr uint8_t MCP_IODIRB   = 0x01;
constexpr uint8_t MCP_GPINTENA = 0x04;
constexpr uint8_t MCP_GPINTENB = 0x05;
constexpr uint8_t MCP_INTCONA  = 0x08;
constexpr uint8_t MCP_INTCONB  = 0x09;
constexpr uint8_t MCP_IOCON    = 0x0A;
constexpr uint8_t MCP_GPPUA    = 0x0C;
constexpr uint8_t MCP_GPPUB    = 0x0D;
constexpr uint8_t MCP_INTFA    = 0x0E;
constexpr uint8_t MCP_INTFB    = 0x0F;
constexpr uint8_t MCP_INTCAPA  = 0x10;
constexpr uint8_t MCP_INTCAPB  = 0x11;
constexpr uint8_t MCP_GPIOA    = 0x12;
constexpr uint8_t MCP_GPIOB    = 0x13;

// Rotary pins (MCP bit positions)
constexpr uint8_t ROTARY_A_PIN = 14;
constexpr uint8_t ROTARY_B_PIN = 15;

// I2C constants
constexpr uint32_t I2C_CLK_SPEED_HZ = 50000;
constexpr uint8_t ALL_INPUTS        = 0xFF;
constexpr gpio_num_t PIN_I2C_ENABLE = GPIO_NUM_17;

namespace buttons {

class MCPInputHandler {
public:
    MCPInputHandler(uint8_t address, i2c_port_t port);

    esp_err_t begin(gpio_num_t sda, gpio_num_t scl, gpio_num_t intPin);

    void setButtonCallback(std::function<void(uint8_t, bool)> cb);
    void setReleaseCallback(std::function<void(uint8_t, bool)> cb);
    void setRotaryCallback(std::function<void(int)> cb);

    void setTimeout(uint32_t ms);
    void I2CEnable(bool enable);
#ifdef DEBUG_MCP_SCAN
    void dumpRegisters() const;
    
    void scanner() const;
    #endif

private:
    // Setup helpers
    esp_err_t initI2CBus(gpio_num_t sda, gpio_num_t scl);
    esp_err_t initMCP23018();
    esp_err_t initInterruptPin();
    void      createInterruptTask();
    void      clearInitialInterrupts();

    // Interrupt handling
    static void gpioISR(void *arg);
    void        interruptTaskLoop();
    void        handleInterrupt();
    void        decodeRotary(uint16_t state);

    // I2C helpers
    esp_err_t i2cWrite(const uint8_t *data, size_t len) const;
    esp_err_t i2cWriteRead(uint8_t reg, uint8_t *data, size_t len) const;
    uint8_t   readRegister(uint8_t reg) const;
    uint16_t  readGPIO16() const;
    void      writeRegister(uint8_t reg, uint8_t val);
    void      writeRegisterPair(uint8_t baseReg, uint8_t a, uint8_t b);

private:
    const uint8_t    i2cAddr;
    const i2c_port_t i2cPort;
    gpio_num_t       interruptPin;

    uint16_t prevState;
    uint8_t  rotaryLast;
    TickType_t ticksToWait;

    TaskHandle_t interruptTaskHandle;
    std::function<void(uint8_t, bool)> buttonCallback;
    std::function<void(uint8_t, bool)> releaseCallback;
    std::function<void(int)>           rotaryCallback;

    static constexpr int8_t ROTARY_TABLE[16] = {
        0, -1, 1, 0,
        1,  0, 0, -1,
       -1,  0, 0, 1,
        0,  1, -1, 0
    };
};

} // namespace buttons
