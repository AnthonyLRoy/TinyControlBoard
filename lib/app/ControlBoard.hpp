#pragma once

#include "powerLed.hpp"
#include "statusLed.hpp"
#include "relay.hpp"
#include "input/input.hpp"
#include "serial.hpp"
#include "spi.hpp"
#include "input/actions/actionsResponse.hpp"
#include "app/actionProcessor.hpp"
#include "app/ControlBoardActionRegistry.hpp"
#include "app/ControlBoardBootstrap.hpp"
#include "app/ControlBoardInputDispatcher.hpp"
#include "app/SerialHeartbeatRouter.hpp"
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
        bool init();
        void deinit();
        ActionProcessor &getActionProcessor() { return *mpResponseProcessor; }

    private:
        void process(const actions::ActionResponse &response) override;
        void setActivityStatus(ControlBoardWorkingStatus status) override;
        void setButtonLed(uint8_t pin, bool enabled) override;
        void handleHeartbeatReceived() override;
        void handleSerialRxMessage(const UartMessage &rMsg);

        serialBus::Serial *mpSerialHandler = nullptr;
        relays::StandardRelay *mpRelays = nullptr;
        std::unique_ptr<ActionProcessor> mpResponseProcessor;
        std::unique_ptr<ControlBoardInputDispatcher> mpInputDispatcher;
        std::unique_ptr<SerialHeartbeatRouter> mpHeartbeatRouter;
        ControlBoardActionRegistry mActionRegistry;
        ControlBoardBootstrap mBootstrap;

        buttons::McpInputHandler mMcpHandler{board::i2c::kMcpAddress, I2C_NUM_0};
        std::array<actions::ButtonAction *, board::buttons::kCount> mpButtonActions{};
    };
}