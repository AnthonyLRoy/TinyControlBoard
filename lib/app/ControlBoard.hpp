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
        ActionProcessor &getActionProcessor() { return *mpResponseProcessor; }

    private:
        void process(const actions::ActionResponse &response) override;
        void setActivityStatus(ControlBoardWorkingStatus status) override;
        void setButtonLed(uint8_t pin, bool enabled) override;
        void handleHeartbeatReceived() override;
        void handleSerialRxMessage(const UartMessage &rMsg);
        bool enqueueButtonEvent(const ButtonEvent &event, const char *pEventName);

        static constexpr uint8_t kButtonQueueDepth = 16;
        static void actionTask(void *pvParam);

        transport::uart::UartTransport *mpSerialHandler = nullptr;
        relays::StandardRelay *mpRelays = nullptr;
        std::unique_ptr<ActionProcessor> mpResponseProcessor;
        std::unique_ptr<ControlBoardInputDispatcher> mpInputDispatcher;
        std::unique_ptr<SerialHeartbeatRouter> mpHeartbeatRouter;
        ControlBoardActionRegistry mActionRegistry;
        ControlBoardBootstrap mBootstrap;
        ControlBoardWorkingStatus mBackgroundStatus = ControlBoardWorkingStatus::Idle;

        buttons::McpInputHandler mMcpHandler{board::i2c::kMcpAddress, I2C_NUM_0};
        ControlBoardInputDispatcher::ActionMap mpButtonActions{};

        SystemState mSystemState;
        QueueHandle_t mButtonEventQueue = nullptr;
        TaskHandle_t mActionTaskHandle = nullptr;
        uint32_t mDroppedButtonEvents = 0;
    };
}