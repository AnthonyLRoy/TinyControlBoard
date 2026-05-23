#include "app/actionProcessor.hpp"

#include "app/ActionCommandRoutingPolicy.hpp"
#include "powerLed.hpp"
#include <inttypes.h>

namespace controlSystem
{
    ActionProcessor::ActionProcessor(transport::uart::UartTransport &rSerialBus, relays::StandardRelay &rRelays, SystemState &rSystemState, IActivityStatusSink *p_activitySink)
        : mr_serial(rSerialBus), mr_relays(rRelays), mr_systemState(rSystemState)
    {
        mp_serialUartCommandSink = std::make_unique<SerialUartCommandSink>(mr_serial);
        mp_actionUartDispatcher = std::make_unique<ActionUartDispatcher>(*mp_serialUartCommandSink);
        mp_rpiBootManager = std::make_unique<RpiBootManager>();
        mp_relayController = std::make_unique<RelayController>(mr_serial, mr_relays);
        mp_powerStateTransitionHandler = std::make_unique<PowerStateTransitionHandler>(
            mr_serial,
            *mp_relayController,
            *mp_rpiBootManager,
            p_activitySink);
    }

    void ActionProcessor::process(const actions::Action &action)
    {
        ESP_LOGI(k_logTag, "Action processor received command: 0x%04X", action.command);

        // Power gate: separate from routing, applied before the dispatch switch.
        const auto powerState = mr_systemState.powerState.load();
        if (action.route != ActionCommandRoute::PowerStateTransition
            && powerState != ControlBoardPowerState::ON)
        {
            ESP_LOGI(k_logTag, "Ignoring command %u as system is not ON", action.command);
            return;
        }

        switch (action.route)
        {
        case ActionCommandRoute::None:
            return;

        case ActionCommandRoute::PowerStateTransition:
            ESP_LOGI(k_logTag, "Processing power-state transition command");
            handleCommandPowerStateChange(action);
            mr_systemState.powerState.store(indicators::getPowerLed().getState());
            return;

        case ActionCommandRoute::System:
            handleSystemCommand(action);
            return;

        case ActionCommandRoute::Relay:
            handleRelayCommand(action);
            return;

        case ActionCommandRoute::Display:
            handleDisplayCommand(action);
            return;

        case ActionCommandRoute::Brightness:
            handleBrightnessCommand(action);
            return;

        case ActionCommandRoute::UartDispatch:
            if (mp_actionUartDispatcher)
            {
                mp_actionUartDispatcher->handle(action);
            }
            return;

        case ActionCommandRoute::IgnoreWhileNotOn:
            // Never stored on an Action; power gate above handles this path.
            return;
        }
    }

    bool ActionProcessor::handleInboundUartMessage(const UartMessage &message)
    {
        // Scaffold only: protocol-specific inbound handling will be added in a
        // dedicated pass once ACK/STATUS semantics are finalized.
        ESP_LOGI(k_logTag, "Inbound UART message received (type=%u cmd=0x%04X seq=%u)",
                 message.msgType, message.commandId, message.sequence);
        return false;
    }

    bool ActionProcessor::handleSystemCommand(const actions::Action &action)
    {
        if (action.command == CMD_SYS_RPI_SHUTDOWN)
        {
            mr_serial.sendUartCommand("RPi_Shutdown", CMD_SYS_RPI_SHUTDOWN);
            mp_relayController->shutdownRpi(true);
            return true;
        }

        if (action.command == CMD_EXIT_ITEM)
        {
            ESP_LOGI(k_logTag, "Sending exit-item message");
            return true;
        }

        return false;
    }

    bool ActionProcessor::handleRelayCommand(const actions::Action &action)
    {
        if (action.command != CMD_TOGGLE_DAC_ON && action.command != CMD_TOGGLE_DAC_OFF)
        {
            return false;
        }

        mp_relayController->handleToggleDac(action.command == CMD_TOGGLE_DAC_ON);
        return true;
    }

    bool ActionProcessor::handleDisplayCommand(const actions::Action &action)
    {
        if (action.command != CMD_DISPLAY_OFF && action.command != CMD_DISPLAY_ON)
        {
            return false;
        }

        ESP_LOGI(k_logTag, "Processing display toggle command (%s)",
                 action.command == CMD_DISPLAY_ON ? "ON" : "OFF");

        indicators::getMonitorBrightnessController().setBlanked(action.command == CMD_DISPLAY_OFF);
        return true;
    }

    bool ActionProcessor::handleBrightnessCommand(const actions::Action &action)
    {
        if (action.command != CMD_CYCLE_BRIGHTNESS)
        {
            return false;
        }

        indicators::getMonitorBrightnessController().cycleBrightness();
        ESP_LOGI(k_logTag, "Cycling monitor brightness");
        return true;
    }

    void ActionProcessor::handleHeartbeatReceived()
    {
        if (mp_rpiBootManager)
        {
            mp_rpiBootManager->handleHeartbeatReceived();
        }
    }

    void ActionProcessor::handleHeartbeatTimeout()
    {
        if (mp_rpiBootManager)
        {
            mp_rpiBootManager->handleHeartbeatTimeout();
        }
    }

    bool ActionProcessor::waitForRpiToBoot(uint32_t timeoutMs)
    {
        if (mp_rpiBootManager)
        {
            return mp_rpiBootManager->waitForRpiToBoot(timeoutMs);
        }
        return false;
    }

    bool ActionProcessor::waitForRpiShutdown(uint32_t timeoutMs)
    {
        if (mp_rpiBootManager)
        {
            return mp_rpiBootManager->waitForRpiShutdown(timeoutMs);
        }
        return false;
    }

    bool ActionProcessor::handleCommandPowerStateChange(const actions::Action &action)
    {
        return mp_powerStateTransitionHandler->handle(action);
    }

    bool ActionProcessor::triggerInitialPowerOn()
    {
        // Force LED to OFF so PowerStateTransitionPolicy treats this as a power-on request
        indicators::getPowerLed().setState(ControlBoardPowerState::OFF);
        actions::Action syntheticAction;
        syntheticAction.command = CMD_SYS_POWER;
        syntheticAction.releaseTimeMillis = 0;
        const bool result = mp_powerStateTransitionHandler->handle(syntheticAction);
        mr_systemState.powerState.store(indicators::getPowerLed().getState());
        return result;
    }
}