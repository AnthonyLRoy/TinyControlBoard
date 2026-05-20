#pragma once

#include "powerLed.hpp"
#include "statusLed.hpp"
#include "relay.hpp"
#include "input/input.hpp"
#include "transport/uart/serial.hpp"
#include "input/actions/actionsResponse.hpp"
#include "app/actionProcessor.hpp"
#include "app/ControlBoardActionRegistry.hpp"
#include "app/ControlBoardBootstrap.hpp"
#include "app/ControlBoardInputDispatcher.hpp"
#include "app/SerialHeartbeatRouter.hpp"
#include "app/ButtonEventQueue.hpp"
#include "app/SystemState.hpp"
#include "board/boardConfig.hpp"
#include "freertos/FreeRTOS.h"
#include <array>
#include <memory>

namespace controlSystem
{
    class ControlBoard : private IActionResponseSink,
                         private IControlBoardIndicators,
                         private IHeartbeatSink
    {
    public:
        bool init();
        void deinit();
        ActionProcessor &getActionProcessor() { return *mp_responseProcessor; }

    private:
        void initNvs();
        void initTransport();
        void initComponents();

        void process(const actions::ActionResponse &response) override;
        void setActivityStatus(ControlBoardWorkingStatus status) override;
        void setButtonLed(uint8_t pin, bool enabled) override;
        void handleHeartbeatReceived() override;
        void handleSerialRxMessage(const UartMessage &rMsg);

        transport::uart::UartTransport *mp_serialHandler = nullptr;  // non-owning; singleton assigned in initTransport()
        relays::StandardRelay *mp_relays = nullptr;                   // non-owning; singleton assigned in initTransport()
        std::unique_ptr<ActionProcessor> mp_responseProcessor;
        std::unique_ptr<ControlBoardInputDispatcher> mp_inputDispatcher;
        std::unique_ptr<SerialHeartbeatRouter> mp_heartbeatRouter;
        ControlBoardActionRegistry m_actionRegistry;
        ControlBoardBootstrap m_bootstrap;

        buttons::McpInputHandler m_mcpHandler{board::i2c::k_mcpAddress, I2C_NUM_0};
        ControlBoardInputDispatcher::ActionMap mp_buttonActions{};
        ButtonEventQueue m_buttonQueue;

        SystemState m_systemState;
    };
}