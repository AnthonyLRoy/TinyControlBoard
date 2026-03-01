#pragma once


#include <cstdint>
#include <functional>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_Check.h"
#include <esp_log.h>
#include "project_cfg.hpp"





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
 // Rotary movement detection On device Index


constexpr uint8_t ROTARY_A_PIN = 13;  // Rotary A pin
constexpr uint8_t ROTARY_B_PIN = 14;  // Rotary B pin 
constexpr uint8_t ROTARY_ACTION = 13;  // Combined Rotary pin for detection 

// I2C constants
constexpr uint32_t I2C_CLK_SPEED_HZ = 50000;
constexpr uint8_t ALL_INPUTS        = 0xFF;
constexpr gpio_num_t PIN_I2C_ENABLE = GPIO_NUM_17;

namespace buttons {

class McpInputHandler {
public:
    McpInputHandler(uint8_t address, i2c_port_t port);

    esp_err_t begin(gpio_num_t sda, gpio_num_t scl, gpio_num_t intPin);

    void setButtonCallback(std::function<void(uint8_t, bool)> cb);
    void setReleaseCallback(std::function<void(uint8_t, bool)> cb);
    void setRotaryCallback(std::function<void(int)> cb);

    void setTimeout(uint32_t ms);
    void enableI2c(bool enable);
#ifdef DEBUG_MCP_SCAN
    void dumpRegisters() const;
    
    void scanI2c() const;
    #endif

private:
    // Setup helpers
    esp_err_t initI2cBus(gpio_num_t sda, gpio_num_t scl);
    esp_err_t initMcp23018();
    esp_err_t initInterruptPin();
    void      createInterruptTask();
    void      clearInitialInterrupts();

    // Interrupt handling
    static void gpioIsr(void *pArg);
    void        runInterruptTaskLoop();
    void        handleInterrupt();
    void        decodeRotary(uint16_t state);

    // I2C helpers
    esp_err_t i2cWrite(const uint8_t *pData, size_t len) const;
    esp_err_t i2cWriteRead(uint8_t reg, uint8_t *pData, size_t len) const;
    uint8_t   readRegister(uint8_t reg) const;
    uint16_t  readGpio16() const;
    void      writeRegister(uint8_t reg, uint8_t val);
    void      writeRegisterPair(uint8_t baseReg, uint8_t a, uint8_t b);

private:
    const uint8_t    mI2cAddr;
    const i2c_port_t mI2cPort;
    gpio_num_t       mInterruptPin;

    uint16_t mPrevState;
    uint8_t  mRotaryLast;
    TickType_t mTicksToWait;

    TaskHandle_t mpInterruptTaskHandle;
    std::function<void(uint8_t, bool)> mButtonCallback;
    std::function<void(uint8_t, bool)> mReleaseCallback;
    std::function<void(int)>           mRotaryCallback;

    static constexpr int8_t msRotaryTable[16] = {
        0, -1, 1, 0,
        1,  0, 0, -1,
       -1,  0, 0, 1,
        0,  1, -1, 0
    };
};

} // namespace buttons
