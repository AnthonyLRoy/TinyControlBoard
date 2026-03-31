#pragma once


#include "powerLed.hpp"
#include "statusLed.hpp"
#include "relay.hpp"
#include "mcpInputHandler.hpp"
#include "serial.hpp"
#include "spi.hpp"
#include "buttonActions.hpp"
#include "actionsResponse.hpp"
#include "actionProcessor.hpp"
#include "board/boardConfig.hpp"
#include <array>
#include <memory>

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
        ActionProcessor& getActionProcessor() { return *mpResponseProcessor; }

    private:

        bool setupRelays();
        bool setupSerial();
        bool setupMcpHandler();
        void setupMcpCallbacks();
        void createButtonActionMap();
        
        // MCP Callback handlers
        void handleButtonPressed(uint8_t pin);
        void handleButtonReleased(uint8_t pin);
        void handleRotaryMovement(int movement);
        
        // Serial/UART Callback handler
        void handleSerialRxMessage(const UartMessage &rMsg);

        // Members - raw pointers not using smart pointers a) because i don't understand them and don't need them because nothing is deleted 
        serialBus::Serial *mpSerialHandler = nullptr;
        relays::StandardRelay *mpRelays = nullptr;
        std::unique_ptr<ActionProcessor> mpResponseProcessor;

        //declare handler and button action fucntions
        buttons::McpInputHandler mMcpHandler{board::i2c::kMcpAddress, I2C_NUM_0};
        std::array<actions::ButtonAction *, board::buttons::kCount> mpButtonActions{};
    };
}