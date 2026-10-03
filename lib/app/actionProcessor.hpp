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
        // Creates the action processor with board serial, relay, status, and runtime state references.
        ActionProcessor(transport::uart::UartTransport &rSerialBus,
                        relays::StandardRelay &rRelays,
                        SystemState &rSystemState,
                        IActivityStatusSink *p_activitySink = nullptr,
                        std::function<void(uint8_t)> onRemoteToggle = {});
        /// isRemoteOrigin: true when the action came from outside the physical button
        /// path (e.g. injectCommand). Physical button presses already toggle their LED
        /// in ControlBoardInputDispatcher, so passing true there would double-toggle it.
        // Executes a queued action while optionally suppressing duplicate remote LED toggles.
        void process(std::unique_ptr<actions::IAction> iaction, bool isRemoteOrigin = false);
        // Handles an inbound UART control message and routes it through the action pipeline.
        bool handleInboundUartMessage(const UartMessage &message);

        /// Thread-safe entry point for injecting commands from external tasks (e.g. BLE).
        /// Best-effort: if the processor is busy, the command is dropped.
        // Adds a command to the processor from an external task without waiting for the main loop.
        void injectCommand(CommandId cmd, uint16_t releaseMs = 0);

        // Marks a valid Raspberry Pi heartbeat as received and updates board status.
        void handleHeartbeatReceived();
        // Handles the case where the Raspberry Pi heartbeat timed out and the board must react.
        void handleHeartbeatTimeout();
        // Clears toggled per-button state so the next action stream starts clean.
        void resetToggleStates();
        // Runs the initial board power-on sequence to bring the system into its active state.
        bool triggerInitialPowerOn();
        // Waits for the Raspberry Pi to finish its boot sequence before continuing startup.
        bool waitForRpiToBoot(uint32_t timeoutMs = 60000);
        // Waits for the Raspberry Pi to signal a safe shutdown before finishing power-down.
        bool waitForRpiShutdown(uint32_t timeoutMs = 60000);

    private:
        // Re-applies the NVS-saved DAC select state through the normal action pipeline.
        void restoreDacState();

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