#pragma once

#include "powerLed.hpp"
#include "activeLed.hpp"
#include "relay.hpp"
#include "mcpInputHandler.hpp"
#include "Serial.hpp"
#include "spi.hpp"
#include "buttonActions.hpp"
#include "actionsResponse.hpp"
namespace actions
{
    class ButtonAction;
}
namespace controlSystem
{
    class ControlBoard
    {
    public:
        void init();

    private:
        void SetupRelays();
        void SetupSPI();
        void SetupSerial();
        void SetupMCPHandler();
        void SetupMCPCallbacks();
        void SetupButtonActions();

        // Static pin and config values
        static constexpr gpio_num_t PIN_SPI_DATA = GPIO_NUM_10;
        static constexpr gpio_num_t PIN_SPI_CLK = GPIO_NUM_12;
        static constexpr gpio_num_t PIN_SPI_LATCH = GPIO_NUM_14;

        static constexpr gpio_num_t PIN_SERIAL_TX = GPIO_NUM_2;
        static constexpr gpio_num_t PIN_SERIAL_RX = GPIO_NUM_1;

        static constexpr gpio_num_t PIN_I2C_SCL = GPIO_NUM_15;
        static constexpr gpio_num_t PIN_I2C_SDA = GPIO_NUM_16;
        static constexpr gpio_num_t PIN_I2C_INT = GPIO_NUM_18;

        static constexpr uint8_t MCP_ADDRESS = 0x20;
        static constexpr uart_port_t UART_NUM = UART_NUM_2;

        // Internal module instances
        spibus::SPI spi2 = spibus::SPI(SPI2_HOST);
        serialBus::Serial serialHandler;
        buttons::MCPInputHandler mcpHandler = buttons::MCPInputHandler(MCP_ADDRESS, I2C_NUM_0);
        actions::ButtonAction *buttonActions[16] = {nullptr};
    };
}
