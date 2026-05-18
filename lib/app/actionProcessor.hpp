#pragma once

#include "relay.hpp"
#include "input/actions/actionsResponse.hpp"
#include "app/ActionUartDispatcher.hpp"
#include "app/SerialUartCommandSink.hpp"
#include "app/SystemState.hpp"
#include "esp_log.h"
#include "power/powerState.hpp"
#include "power/PowerStateTransitionHandler.hpp"
#include "power/RPIBootManager.hpp"
#include "power/RelayController.hpp"
#include <driver/gpio.h>
#include "indicators/ledManager.hpp"
#include "activityStatus.hpp"
#include "protocol/uartProtocol.hpp"
#include "transport/uart/serial.hpp"
#include <memory>

namespace controlSystem
{
    class ActionProcessor
    {
    public:
        ActionProcessor(transport::uart::UartTransport &rSerialBus,
                        relays::StandardRelay &rRelays,
                        SystemState &rSystemState,
                        IActivityStatusSink *pActivitySink = nullptr);
        void process(const actions::ActionResponse &response);
        bool handleInboundUartMessage(const UartMessage &message);
        const char *getCommandNameForPin(uint8_t pin);

        void handleHeartbeatReceived();
        void handleHeartbeatTimeout();
        bool waitForRpiToBoot(uint32_t timeoutMs = 60000);
        bool waitForRpiShutdown(uint32_t timeoutMs = 60000);

    private:
        bool handleCommandPowerStateChange(const actions::ActionResponse &response);
        bool handleSystemCommand(const actions::ActionResponse &response);
        bool handleRelayCommand(const actions::ActionResponse &response);
        bool handleDisplayCommand(const actions::ActionResponse &response);
        bool handleBrightnessCommand(const actions::ActionResponse &response);

        std::unique_ptr<ActionUartDispatcher> mpActionUartDispatcher;
        std::unique_ptr<SerialUartCommandSink> mpSerialUartCommandSink;
        std::unique_ptr<RpiBootManager> mpRpiBootManager;
        std::unique_ptr<RelayController> mpRelayController;
        std::unique_ptr<PowerStateTransitionHandler> mpPowerStateTransitionHandler;

        transport::uart::UartTransport &mrSerial;
        relays::StandardRelay &mrRelays;
        SystemState &mrSystemState;

        static constexpr const char *kLogTag = "Action_Processor";
    };
}