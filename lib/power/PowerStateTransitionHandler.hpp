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
        // Creates the power-state machine with references to serial, relays, boot tracking, and status sinks.
        PowerStateTransitionHandler(transport::uart::UartTransport &rSerial,
                                    RelayController &rRelayController,
                                    RpiBootManager &rRpiBootManager,
                                    SystemState &rSystemState,
                                    IActivityStatusSink *p_activitySink = nullptr);

        /// Evaluates the requested power transition and executes the appropriate
        /// sequence. Returns the resulting ControlBoardPowerState.
        // Executes a power-state action and returns the resulting board power state.
        ControlBoardPowerState handle(const actions::IAction &action);

    private:
        // Reports a new working status to the board's activity indicator sink.
        void reportStatus(ControlBoardWorkingStatus status);
        /// Shared preamble for both Sleep and DeepSleep: notifies the RPi,
        /// waits for its shutdown, then cuts the RPi and 3V3 relays.
        // Runs the common Raspberry Pi shutdown sequence before cutting power rail relays.
        void runRpiShutdownSequence();
        /// Publishes the new state to both the LED indicator and the BLE-visible
        /// SystemState so remote clients see intermediate transitions in real time.
        // Updates the board state and LED indicators so remote clients see the current power transition.
        void publishPowerState(ControlBoardPowerState state);

        transport::uart::UartTransport &mr_serial;
        RelayController &mr_relayController;
        RpiBootManager &mr_rpiBootManager;
        SystemState &mr_systemState;
        IActivityStatusSink *mp_activitySink;

        static constexpr const char *k_logTag = "Power_State_Hdlr";
    };
}