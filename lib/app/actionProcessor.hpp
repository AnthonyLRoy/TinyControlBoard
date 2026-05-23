#pragma once

#include "relay.hpp"
#include "input/actions/actionsResponse.hpp"
#include "app/ActionFactory.hpp"
#include "app/ActionContext.hpp"
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
                        IActivityStatusSink *p_activitySink = nullptr);
        void process(const actions::Action &action);
        bool handleInboundUartMessage(const UartMessage &message);
        const char *getCommandNameForPin(uint8_t pin);

        void handleHeartbeatReceived();
        void handleHeartbeatTimeout();
        bool triggerInitialPowerOn();
        bool waitForRpiToBoot(uint32_t timeoutMs = 60000);
        bool waitForRpiShutdown(uint32_t timeoutMs = 60000);

    private:
        std::unique_ptr<ActionUartDispatcher> mp_actionUartDispatcher;
        std::unique_ptr<SerialUartCommandSink> mp_serialUartCommandSink;
        std::unique_ptr<RpiBootManager> mp_rpiBootManager;
        std::unique_ptr<RelayController> mp_relayController;
        std::unique_ptr<PowerStateTransitionHandler> mp_powerStateTransitionHandler;

        transport::uart::UartTransport &mr_serial;
        relays::StandardRelay &mr_relays;
        SystemState &mr_systemState;

        static constexpr const char *k_logTag = "Action_Processor";
    };
}