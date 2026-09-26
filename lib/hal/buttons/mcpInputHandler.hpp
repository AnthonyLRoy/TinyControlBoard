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
inline constexpr gpio_num_t PIN_I2C_ENABLE = board::i2c::k_enablePin;

namespace buttons {

class McpInputHandler {
public:
    // Creates a driver instance bound to a specific MCP23018 address and I2C bus.
    McpInputHandler(uint8_t address, i2c_port_t port);

    // Initializes the I2C bus, the MCP23018 chip, and the interrupt handling task.
    esp_err_t begin(gpio_num_t sda, gpio_num_t scl, gpio_num_t intPin);

    // Registers the callback used when a button transitions to the pressed state.
    void setButtonCallback(std::function<void(uint8_t, bool)> cb);
    // Registers the callback used when a button transitions to the released state.
    void setReleaseCallback(std::function<void(uint8_t, bool)> cb);
    // Registers the callback used when the rotary encoder reports movement.
    void setRotaryCallback(std::function<void(int)> cb);

    // Updates the I2C transaction timeout used while reading and writing the MCP device.
    void setTimeout(uint32_t ms);
    // Enables or disables the I2C power rail connected to the MCP board.
    void enableI2c(bool enable);
#ifdef DEBUG_MCP_SCAN
    // Dumps the register map for debugging MCP register state.
    void dumpRegisters() const;
    // Scans the I2C bus and logs any connected devices.
    void scanI2c() const;
#endif

private:
    // Configures the ESP32 as the I2C master for the MCP23018 bus.
    esp_err_t initI2cBus(gpio_num_t sda, gpio_num_t scl);
    // Configures the MCP23018 input-register defaults and interrupt behavior.
    esp_err_t initMcp23018();
    // Configures the GPIO interrupt line used by the MCP23018 to wake the task.
    esp_err_t initInterruptPin();
    // Creates the FreeRTOS task that drains GPIO edge notifications from the MCP.
    void createInterruptTask();
    // Reads the current GPIO state to establish a clean baseline before processing events.
    void clearInitialInterrupts();

    // ISR entry point that wakes the MCP interrupt task from interrupt context.
    static void gpioIsr(void *p_arg);
    // Runs the background loop that consumes interrupt notifications and processes changes.
    void runInterruptTaskLoop();
    // Reads the changed pin states and dispatches button and rotary events.
    void handleInterrupt();
    // Converts the raw rotary quadrature state into a signed movement amount.
    void decodeRotary(uint16_t state);

    // Writes a raw I2C buffer to the MCP23018 register map.
    esp_err_t i2cWrite(const uint8_t *p_data, size_t len) const;
    // Writes a register address then reads back the requested data bytes.
    esp_err_t i2cWriteRead(uint8_t reg, uint8_t *p_data, size_t len) const;
    // Reads a single register byte from the MCP23018.
    uint8_t readRegister(uint8_t reg) const;
    // Reads both GPIO ports as a single 16-bit state value.
    uint16_t readGpio16() const;
    // Writes a single byte value into one MCP23018 register.
    void writeRegister(uint8_t reg, uint8_t val);
    // Writes a two-byte register pair in one transaction for adjacent GPIO settings.
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