#pragma once

#include "input/actions/IAction.hpp"
#include "power/DelayedBootRecoveryState.hpp"
#include "power/RelayController.hpp"
#include "power/RPIBootManager.hpp"
#include "power/powerState.hpp"
#include "hal/uart/serial.hpp"
#include "indicators/activityStatus.hpp"

namespace controlSystem
{
    class PowerStateTransitionHandler
    {
    public:
        PowerStateTransitionHandler(transport::uart::UartTransport &rSerial,
                                    RelayController &rRelayController,
                                    RpiBootManager &rRpiBootManager,
                                    IActivityStatusSink *p_activitySink = nullptr);

        /// Evaluates the requested power transition and executes the appropriate
        /// sequence. Returns the resulting ControlBoardPowerState.
        ControlBoardPowerState handle(const actions::IAction &action);
        bool isAwaitingLateBootHeartbeat() const;
        bool completePendingBootOnHeartbeat();

    private:
        void enterOnState();
        void reportStatus(ControlBoardWorkingStatus status);
        /// Shared preamble for both Sleep and DeepSleep: notifies the RPi,
        /// waits for its shutdown, then cuts the RPi and screen relays.
        void runRpiShutdownSequence();
        /// Sets both the power LED and monitor brightness controller to the
        /// given state in one call.
        void setIndicatorState(ControlBoardPowerState state);

        transport::uart::UartTransport &mr_serial;
        RelayController &mr_relayController;
        RpiBootManager &mr_rpiBootManager;
        IActivityStatusSink *mp_activitySink;
        DelayedBootRecoveryState m_delayedBootRecovery;

        static constexpr const char *k_logTag = "Power_State_Hdlr";
    };
}