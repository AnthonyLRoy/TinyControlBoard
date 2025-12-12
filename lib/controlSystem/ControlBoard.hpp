#pragma once


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

namespace actions {
    class ButtonAction;
}

namespace controlSystem
{
    // Configuration constants
    struct ControlBoardConfig
    {
        // UART Configuration
        static constexpr uint32_t UART_BOARD_RATE = 115200;
        
        // Heartbeat Configuration
        static constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 15000;
        static constexpr uint32_t INIT_DELAY_MS = 12000;  // Power LED transition delay
        
        // Pin Configuration
        static constexpr gpio_num_t PIN_SERIAL_TX = GPIO_NUM_2;
        static constexpr gpio_num_t PIN_SERIAL_RX = GPIO_NUM_1;
        static constexpr gpio_num_t PIN_I2C_SCL = GPIO_NUM_15;
        static constexpr gpio_num_t PIN_I2C_SDA = GPIO_NUM_16;
        static constexpr gpio_num_t PIN_I2C_INT = GPIO_NUM_18;
        
        // MCP Configuration
        static constexpr uint8_t MCP_ADDRESS = 0x20;
        static constexpr uart_port_t UART_NUM = UART_NUM_2;
        static constexpr int MCP_TIMEOUT_MS = 10;
        
        // Button Action Indices
        static constexpr uint8_t BTN_POWER = 0;
        static constexpr uint8_t BTN_PREV_TRACK = 1;
        static constexpr uint8_t BTN_NEXT_TRACK = 2;
        static constexpr uint8_t BTN_SKIP_FORWARD = 3;
        static constexpr uint8_t BTN_SKIP_BACK = 4;
        static constexpr uint8_t BTN_PLAY_PAUSE = 5;
        static constexpr uint8_t BTN_STOP = 6;
        static constexpr uint8_t BTN_PREV_MENU = 7;
        static constexpr uint8_t BTN_NEXT_MENU = 8;
        static constexpr uint8_t BTN_MENU_SELECT = 9;
        static constexpr uint8_t BTN_TOGGLE_DAC = 10;
        static constexpr uint8_t BTN_TOGGLE_DISPLAY = 11;
        static constexpr uint8_t BTN_TOGGLE_METER = 12;
        static constexpr uint8_t BTN_ROTARY_EVENT_1 = 13;
        static constexpr uint8_t BTN_ROTARY_EVENT_2 = 14;
        
        static constexpr uint8_t NUM_BUTTONS = 15;
    };

    class ControlBoard
    {
    public:
        bool init();            // returns true if everything initialized successfully
        void deinit();          // optional cleanup
        actionProcessor& getActionProcessor() { return *responseProcessor; }

    private:
        // Initialization helpers
        uint16_t spiPintActiveBitMap = 0x0000; // Bitmap to track active SPI pins
        
        bool setupRelays();
        bool setupSerial();
        bool setupMCPHandler();
        void setupMCPCallbacks();
        void createButtonActionMap();
        
        // MCP Callback handlers
        void handleButtonPressed(uint8_t pin);
        void handleButtonReleased(uint8_t pin);
        void handleRotaryMovement(int movement);

        // Members - raw pointers not using smart pointers a) because i don't understand them and don't need them because nothing is deleted 
        serialBus::Serial* serialHandler = nullptr;
        relays::StandardRelay* relays = nullptr;
        spibus::SPI* spi = nullptr;
        actionProcessor* responseProcessor = nullptr;

        //declare handler and button action fucntions
        buttons::MCPInputHandler mcpHandler{ControlBoardConfig::MCP_ADDRESS, I2C_NUM_0};
        actions::ButtonAction* buttonActions[ControlBoardConfig::NUM_BUTTONS] = {nullptr};
    };
}