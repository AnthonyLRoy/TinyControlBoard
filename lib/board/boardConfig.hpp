#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "board/boardButtonIds.hpp"

namespace board
{

    namespace timing
    {
        inline constexpr uint32_t k_heartbeatTimeoutMs = 30000;
        inline constexpr uint32_t k_initDelayMs = 5000;
        inline constexpr uint32_t k_powerSettleDelayMs = 1500;
        inline constexpr uint32_t k_vcc3v3OnDelayMs = 1000;
        inline constexpr uint32_t k_rpiOnDelayMs = 1000;
        inline constexpr uint32_t k_rpiBootTimeoutMs = 60000;
        inline constexpr uint32_t k_rpiShutdownTimeoutMs = 60000;
        inline constexpr uint32_t k_rpiShutdownSettleDelayMs = 500;
        inline constexpr uint32_t k_vcc3v3PowerOffDelayMs = 5000;
    } // namespace timing

    namespace serial
    {
        inline constexpr uart_port_t k_port = UART_NUM_2;
        inline constexpr uint32_t k_baudRate = 115200;
        inline constexpr gpio_num_t k_txPin = GPIO_NUM_2;
        inline constexpr gpio_num_t k_rxPin = GPIO_NUM_1;
        inline constexpr std::size_t k_bufferSize = 1024;
        inline constexpr gpio_num_t k_rpiDataReadyPin = GPIO_NUM_42;
        inline constexpr gpio_num_t k_esp32DataReadyPin = GPIO_NUM_41;
    } // namespace serial

    namespace i2c
    {
        inline constexpr gpio_num_t k_sclPin = GPIO_NUM_15;
        inline constexpr gpio_num_t k_sdaPin = GPIO_NUM_16;
        inline constexpr gpio_num_t k_interruptPin = GPIO_NUM_18;
        inline constexpr gpio_num_t k_enablePin = GPIO_NUM_17;
        inline constexpr uint8_t k_mcpAddress = 0x20;
        inline constexpr int k_mcpTimeoutMs = 10;
        inline constexpr uint32_t k_clockSpeedHz = 50000;
    } // namespace i2c

    namespace relays
    {
        inline constexpr gpio_num_t k_vcc3v3Power = GPIO_NUM_13;
        inline constexpr gpio_num_t k_dacPower = GPIO_NUM_12;
        inline constexpr gpio_num_t k_rpiPower = GPIO_NUM_11;
        inline constexpr gpio_num_t k_outputStagePower = GPIO_NUM_9;
        inline constexpr gpio_num_t k_essDacEnabled = GPIO_NUM_10;
        inline constexpr gpio_num_t k_general1 = GPIO_NUM_47;
        inline constexpr gpio_num_t k_general2 = GPIO_NUM_39;
    } // namespace relays

    namespace indicators
    {
        inline constexpr gpio_num_t k_appActiveLed = GPIO_NUM_3;
        inline constexpr gpio_num_t k_appStandbyLed = GPIO_NUM_4;
        inline constexpr gpio_num_t k_workingStatusLed = GPIO_NUM_48;
        inline constexpr gpio_num_t k_buttonLedPwmPin = GPIO_NUM_21;
        inline constexpr uint32_t k_buttonLedDefaultDuty = (8191 * 8) / 10;
        inline constexpr gpio_num_t k_monitorBrightness = GPIO_NUM_43;
        inline constexpr gpio_num_t k_spiData = GPIO_NUM_7;
        inline constexpr gpio_num_t k_spiClock = GPIO_NUM_6;
        inline constexpr gpio_num_t k_spiLatch = GPIO_NUM_5;
    } // namespace indicators

    namespace debug
    {
        // Skips waiting for the RPi heartbeat during boot.
        // Automatically true in debug builds, false in release builds.
        // Override by defining SIMULATE_RPI_BOOT=1 or =0 in your build flags.
#if defined(SIMULATE_RPI_BOOT)
        inline constexpr bool k_simulateRpiBoot = (SIMULATE_RPI_BOOT != 0);
#elif defined(NDEBUG)
        inline constexpr bool k_simulateRpiBoot = false;
#else
        inline constexpr bool k_simulateRpiBoot = false;
#endif
    } // namespace debug

    namespace ble
    {
        inline constexpr char k_deviceName[] = "TinyControlBoard";
        inline constexpr int k_maxConnections = 1;
    } // namespace ble

} // namespace board
