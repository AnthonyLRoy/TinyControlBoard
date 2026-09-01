#pragma once

#include "input/actions/IAction.hpp"
#include "power/RelayController.hpp"
#include "power/RPIBootManager.hpp"
#include "power/powerState.hpp"
#include "hal/uart/serial.hpp"
#include "indicators/activityStatus.hpp"
#include "app/SystemState.hpp"

namespace controlSystem
{
    class PowerStateTransitionHandler
    {
    public:
        PowerStateTransitionHandler(transport::uart::UartTransport &rSerial,
                                    RelayController &rRelayController,
                                    RpiBootManager &rRpiBootManager,
                                    SystemState &rSystemState,
                                    IActivityStatusSink *p_activitySink = nullptr);

        /// Evaluates the requested power transition and executes the appropriate
        /// sequence. Returns the resulting ControlBoardPowerState.
        ControlBoardPowerState handle(const actions::IAction &action);

    private:
        void reportStatus(ControlBoardWorkingStatus status);
        /// Shared preamble for both Sleep and DeepSleep: notifies the RPi,
        /// waits for its shutdown, then cuts the RPi and 3V3 relays.
        void runRpiShutdownSequence();
        /// Publishes the new state to both the LED indicator and the BLE-visible
        /// SystemState so remote clients see intermediate transitions in real time.
        void publishPowerState(ControlBoardPowerState state);

        transport::uart::UartTransport &mr_serial;
        RelayController &mr_relayController;
        RpiBootManager &mr_rpiBootManager;
        SystemState &mr_systemState;
        IActivityStatusSink *mp_activitySink;

        static constexpr const char *k_logTag = "Power_State_Hdlr";
    };
}