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

    void ActionProcessor::process(const actions::ActionResponse &response)
    {
        ESP_LOGI(k_logTag, "Action processor received command: 0x%04X", response.command);

        const auto powerState = mr_systemState.powerState.load();
        switch (ActionCommandRoutingPolicy::classify(response.command, powerState))
        {
        case ActionCommandRoute::None:
            return;

        case ActionCommandRoute::PowerStateTransition:
            ESP_LOGI(k_logTag, "Processing power-state transition command");
            handleCommandPowerStateChange(response);
            mr_systemState.powerState.store(indicators::getPowerLed().getState());
            return;

        case ActionCommandRoute::IgnoreWhileNotOn:
            ESP_LOGI(k_logTag, "Ignoring command %u as system is not ON", response.command);
            return;

        case ActionCommandRoute::System:
            handleSystemCommand(response);
            return;

        case ActionCommandRoute::Relay:
            handleRelayCommand(response);
            return;

        case ActionCommandRoute::Display:
            handleDisplayCommand(response);
            return;

        case ActionCommandRoute::Brightness:
            handleBrightnessCommand(response);
            return;

        case ActionCommandRoute::UartDispatch:
            if (mp_actionUartDispatcher)
            {
                mp_actionUartDispatcher->handle(response);
            }
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

    bool ActionProcessor::handleSystemCommand(const actions::ActionResponse &response)
    {
        if (response.command == CMD_SYS_RPI_SHUTDOWN)
        {
            mr_serial.sendUartCommand("RPi_Shutdown", CMD_SYS_RPI_SHUTDOWN);
            mp_relayController->shutdownRpi(true);
            return true;
        }

        if (response.command == CMD_EXIT_ITEM)
        {
            ESP_LOGI(k_logTag, "Sending exit-item message");
            return true;
        }

        return false;
    }

    bool ActionProcessor::handleRelayCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_TOGGLE_DAC_ON && response.command != CMD_TOGGLE_DAC_OFF)
        {
            return false;
        }

        mp_relayController->handleToggleDac(response.command == CMD_TOGGLE_DAC_ON);
        return true;
    }

    bool ActionProcessor::handleDisplayCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_DISPLAY_OFF && response.command != CMD_DISPLAY_ON)
        {
            return false;
        }

        ESP_LOGI(k_logTag, "Processing display toggle command (%s)",
                 response.command == CMD_DISPLAY_ON ? "ON" : "OFF");

        indicators::getMonitorBrightnessController().setBlanked(response.command == CMD_DISPLAY_OFF);
        return true;
    }

    bool ActionProcessor::handleBrightnessCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_CYCLE_BRIGHTNESS)
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

    bool ActionProcessor::handleCommandPowerStateChange(const actions::ActionResponse &response)
    {
        return mp_powerStateTransitionHandler->handle(response);
    }

    bool ActionProcessor::triggerInitialPowerOn()
    {
        // Force LED to OFF so PowerStateTransitionPolicy treats this as a power-on request
        indicators::getPowerLed().setState(ControlBoardPowerState::OFF);
        actions::ActionResponse syntheticResponse;
        syntheticResponse.command = CMD_SYS_POWER;
        syntheticResponse.releaseTimeMillis = 0;
        const bool result = mp_powerStateTransitionHandler->handle(syntheticResponse);
        mr_systemState.powerState.store(indicators::getPowerLed().getState());
        return result;
    }
}