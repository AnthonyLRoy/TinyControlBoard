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
#include "ControlBoardActionRegistry.hpp"
#include "ControlBoardBootstrap.hpp"
#include "ControlBoardInputDispatcher.hpp"
#include "SerialHeartbeatRouter.hpp"
#include "board/boardConfig.hpp"
#include <array>
#include <memory>

namespace actions {
    class ButtonAction;
}

namespace controlSystem
{
    class ControlBoard : private IActionResponseSink,
                         private IControlBoardIndicators,
                         private IHeartbeatSink
    {
    public:
        bool init();            // returns true if everything initialized successfully
        void deinit();          // optional cleanup
        ActionProcessor& getActionProcessor() { return *mpResponseProcessor; }

    private:
        void process(const actions::ActionResponse &response) override;
        void setActivityStatus(ControlBoardWorkingStatus status) override;
        void setButtonLed(uint8_t pin, bool enabled) override;
        void handleHeartbeatReceived() override;

        // Serial/UART Callback handler
        void handleSerialRxMessage(const UartMessage &rMsg);

        // Members - raw pointers not using smart pointers a) because i don't understand them and don't need them because nothing is deleted 
        serialBus::Serial *mpSerialHandler = nullptr;
        relays::StandardRelay *mpRelays = nullptr;
        std::unique_ptr<ActionProcessor> mpResponseProcessor;
        std::unique_ptr<ControlBoardInputDispatcher> mpInputDispatcher;
        std::unique_ptr<SerialHeartbeatRouter> mpHeartbeatRouter;
        ControlBoardActionRegistry mActionRegistry;
        ControlBoardBootstrap mBootstrap;

        //declare handler and button action fucntions
        buttons::McpInputHandler mMcpHandler{board::i2c::kMcpAddress, I2C_NUM_0};
        std::array<actions::ButtonAction *, board::buttons::kCount> mpButtonActions{};
    };
}