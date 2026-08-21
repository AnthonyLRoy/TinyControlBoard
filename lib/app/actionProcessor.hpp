#pragma once

#include "hal/relay/relay.hpp"
#include "input/actions/IAction.hpp"
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
#include "indicators/activityStatus.hpp"
#include "protocol/uartProtocol.hpp"
#include "hal/uart/serial.hpp"
#include <functional>
#include <memory>

namespace controlSystem
{
    class ActionProcessor
    {
    public:
        ActionProcessor(transport::uart::UartTransport &rSerialBus,
                        relays::StandardRelay &rRelays,
                        SystemState &rSystemState,
                        IActivityStatusSink *p_activitySink = nullptr,
                        std::function<void(uint8_t)> onRemoteToggle = {});
        /// isRemoteOrigin: true when the action came from outside the physical button
        /// path (e.g. injectCommand). Physical button presses already toggle their LED
        /// in ControlBoardInputDispatcher, so passing true there would double-toggle it.
        void process(std::unique_ptr<actions::IAction> iaction, bool isRemoteOrigin = false);
        bool handleInboundUartMessage(const UartMessage &message);

        /// Thread-safe entry point for injecting commands from external tasks (e.g. BLE).
        /// Best-effort: if the processor is busy, the command is dropped.
        void injectCommand(CommandId cmd, uint16_t releaseMs = 0);

        void handleHeartbeatReceived();
        void handleHeartbeatTimeout();
        void resetToggleStates();
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
        std::function<void(uint8_t)> m_onRemoteToggle;

        static constexpr const char *k_logTag = "Action_Processor";
    };
}