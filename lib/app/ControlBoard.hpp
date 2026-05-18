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
#include "app/ButtonEvent.hpp"
#include "app/SystemState.hpp"
#include "board/boardConfig.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
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
        void process(const actions::ActionResponse &response) override;
        void setActivityStatus(ControlBoardWorkingStatus status) override;
        void setButtonLed(uint8_t pin, bool enabled) override;
        void handleHeartbeatReceived() override;
        void handleSerialRxMessage(const UartMessage &rMsg);
        bool enqueueButtonEvent(const ButtonEvent &event, const char *p_eventName);

        static constexpr uint8_t k_buttonQueueDepth = 16;
        static void actionTask(void *pvParam);

        transport::uart::UartTransport *mp_serialHandler = nullptr;
        relays::StandardRelay *mp_relays = nullptr;
        std::unique_ptr<ActionProcessor> mp_responseProcessor;
        std::unique_ptr<ControlBoardInputDispatcher> mp_inputDispatcher;
        std::unique_ptr<SerialHeartbeatRouter> mp_heartbeatRouter;
        ControlBoardActionRegistry m_actionRegistry;
        ControlBoardBootstrap m_bootstrap;
        ControlBoardWorkingStatus m_backgroundStatus = ControlBoardWorkingStatus::Idle;

        buttons::McpInputHandler m_mcpHandler{board::i2c::k_mcpAddress, I2C_NUM_0};
        ControlBoardInputDispatcher::ActionMap mp_buttonActions{};

        SystemState m_systemState;
        QueueHandle_t m_buttonEventQueue = nullptr;
        TaskHandle_t m_actionTaskHandle = nullptr;
        uint32_t m_droppedButtonEvents = 0;
    };
}