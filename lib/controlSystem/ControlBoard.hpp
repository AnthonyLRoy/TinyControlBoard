#pragma once
#include "project_config.hpp"
#include "powerLed.hpp"
#include "activeLed.hpp"
#include "relay.hpp"
#include "mcpInputHandler.hpp"
#include "Serial.hpp"
#include "spi.hpp"
#include "buttonActions.hpp"
#include "actionsResponse.hpp"
#include "actionProcessor.hpp"
#include "PowerStateManager.hpp"

#define UART_BOARD_RATE 9600
namespace actions {
    class ButtonAction;
}

namespace controlSystem
{
    class ControlBoard
    {
    public:
        bool init();            // returns true if everything initialized successfully
        void deinit();          // optional cleanup

        spibus::SPI& getSPI() { return *spi; }
        actionProcessor& getActionProcessor() { return *responseProcessor; }

    private:

        uint16_t spiPintActiveBitMap = 0x0000; // Bitmap to track active SPI pins
        
        bool setupRelays();
        bool setupSPI();
        bool setupSerial();
        bool setupMCPHandler();
        void setupMCPCallbacks();
        void setupButtonActions();


        // SPI pins
        static constexpr gpio_num_t PIN_SPI_DATA = GPIO_NUM_6;
        static constexpr gpio_num_t PIN_SPI_CLK = GPIO_NUM_7;
        static constexpr gpio_num_t PIN_SPI_LATCH = GPIO_NUM_5;

        // serial port pins
        static constexpr gpio_num_t PIN_SERIAL_TX = GPIO_NUM_2;
        static constexpr gpio_num_t PIN_SERIAL_RX = GPIO_NUM_1;

        //I2C pins
        static constexpr gpio_num_t PIN_I2C_SCL = GPIO_NUM_15;
        static constexpr gpio_num_t PIN_I2C_SDA = GPIO_NUM_16;
        static constexpr gpio_num_t PIN_I2C_INT = GPIO_NUM_18;

        //Address of MCP2018 chip
        static constexpr uint8_t MCP_ADDRESS = 0x20;
        static constexpr uart_port_t UART_NUM = UART_NUM_2;

        // Members - raw pointers not using smart pointers a) because i don't understand them and don't need them because nothing is deleted 
        serialBus::Serial* serialHandler = nullptr;
        relays::StandardRelay* relays = nullptr;
        spibus::SPI* spi = nullptr;
        actionProcessor* responseProcessor = nullptr;

        //declare handler and button action fucntions
        buttons::MCPInputHandler mcpHandler{MCP_ADDRESS, I2C_NUM_0};
        actions::ButtonAction* buttonActions[16] = {nullptr};
    };
}