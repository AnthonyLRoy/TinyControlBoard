#pragma once

#include <cstdint>
#include <functional>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_Check.h"
#include <esp_log.h>
#include "board/boardConfig.hpp"
#include "project_cfg.hpp"

constexpr uint8_t MCP_IODIRA = 0x00;
constexpr uint8_t MCP_IODIRB = 0x01;
constexpr uint8_t MCP_GPINTENA = 0x04;
constexpr uint8_t MCP_GPINTENB = 0x05;
constexpr uint8_t MCP_INTCONA = 0x08;
constexpr uint8_t MCP_INTCONB = 0x09;
constexpr uint8_t MCP_IOCON = 0x0A;
constexpr uint8_t MCP_GPPUA = 0x0C;
constexpr uint8_t MCP_GPPUB = 0x0D;
constexpr uint8_t MCP_INTFA = 0x0E;
constexpr uint8_t MCP_INTFB = 0x0F;
constexpr uint8_t MCP_INTCAPA = 0x10;
constexpr uint8_t MCP_INTCAPB = 0x11;
constexpr uint8_t MCP_GPIOA = 0x12;
constexpr uint8_t MCP_GPIOB = 0x13;

inline constexpr uint8_t ROTARY_A_PIN = board::buttons::k_rotaryEventLeft;
inline constexpr uint8_t ROTARY_B_PIN = board::buttons::k_rotaryEventRight;
inline constexpr uint8_t ROTARY_ACTION = board::buttons::k_rotaryEventLeft;
inline constexpr uint32_t I2C_CLK_SPEED_HZ = board::i2c::k_clockSpeedHz;
inline constexpr uint8_t ALL_INPUTS = 0xFF;

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
    esp_err_t initI2cBus(gpio_num_t sda, gpio_num_t scl);
    esp_err_t initMcp23018();
    esp_err_t initInterruptPin();
    void createInterruptTask();
    void clearInitialInterrupts();

    static void gpioIsr(void *p_arg);
    void runInterruptTaskLoop();
    void handleInterrupt();
    void decodeRotary(uint16_t state);

    esp_err_t i2cWrite(const uint8_t *p_data, size_t len) const;
    esp_err_t i2cWriteRead(uint8_t reg, uint8_t *p_data, size_t len) const;
    uint8_t readRegister(uint8_t reg) const;
    uint16_t readGpio16() const;
    void writeRegister(uint8_t reg, uint8_t val);
    void writeRegisterPair(uint8_t baseReg, uint8_t a, uint8_t b);

private:
    const uint8_t m_i2cAddr;
    const i2c_port_t m_i2cPort;
    gpio_num_t m_interruptPin;

    uint16_t m_prevState;
    uint8_t m_rotaryLast;
    TickType_t m_ticksToWait;

    TaskHandle_t mp_interruptTaskHandle;
    std::function<void(uint8_t, bool)> m_buttonCallback;
    std::function<void(uint8_t, bool)> m_releaseCallback;
    std::function<void(int)> m_rotaryCallback;

    static constexpr int8_t ms_rotaryTable[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0
    };
};

} // namespace buttons