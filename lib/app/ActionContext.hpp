#pragma once

#include "app/ActionUartDispatcher.hpp"
#include "power/PowerStateTransitionHandler.hpp"
#include "power/RelayController.hpp"
#include "hal/uart/serial.hpp"
#include "app/SystemState.hpp"
#include "indicators/powerLed.hpp"
#include "indicators/monitorBrightnessController.hpp"

namespace controlSystem
{
    /// All services that a concrete IAction::execute() implementation may need.
    /// Constructed locally in ActionProcessor::process() and passed by reference
    /// so execute() has no dependency on ActionProcessor itself.
    struct ActionContext
    {
        ActionUartDispatcher                    &uartDispatcher;
        PowerStateTransitionHandler             &powerHandler;
        RelayController                         &relayController;
        transport::uart::UartTransport          &serial;
        SystemState                             &systemState;
        indicators::PowerLed                    &powerLed;
        indicators::MonitorBrightnessController &brightnessController;
    };
}
