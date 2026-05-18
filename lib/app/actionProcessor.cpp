#include "app/actionProcessor.hpp"

#include "app/ActionCommandRoutingPolicy.hpp"
#include "powerLed.hpp"
#include <inttypes.h>

namespace controlSystem
{
    ActionProcessor::ActionProcessor(transport::uart::UartTransport &rSerialBus, relays::StandardRelay &rRelays, SystemState &rSystemState, IActivityStatusSink *pActivitySink)
        : mrSerial(rSerialBus), mrRelays(rRelays), mrSystemState(rSystemState)
    {
        mpSerialUartCommandSink = std::make_unique<SerialUartCommandSink>(mrSerial);
        mpActionUartDispatcher = std::make_unique<ActionUartDispatcher>(*mpSerialUartCommandSink);
        mpRpiBootManager = std::make_unique<RpiBootManager>();
        mpRelayController = std::make_unique<RelayController>(mrSerial, mrRelays);
        mpPowerStateTransitionHandler = std::make_unique<PowerStateTransitionHandler>(
            mrSerial,
            *mpRelayController,
            *mpRpiBootManager,
            pActivitySink);
    }

    void ActionProcessor::process(const actions::ActionResponse &response)
    {
        ESP_LOGI(mspTag, "Action Processor received command: 0x%04X", response.command);

        const auto powerState = mrSystemState.powerState.load();
        switch (ActionCommandRoutingPolicy::classify(response.command, powerState))
        {
        case ActionCommandRoute::None:
            return;

        case ActionCommandRoute::PowerStateTransition:
            ESP_LOGI(mspTag, "Processing Power State Change Command");
            handleCommandPowerStateChange(response);
            mrSystemState.powerState.store(indicators::getPowerLed().getState());
            return;

        case ActionCommandRoute::IgnoreWhileNotOn:
            ESP_LOGI(mspTag, "Ignoring command %u as system is not ON", response.command);
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
            if (mpActionUartDispatcher)
            {
                mpActionUartDispatcher->handle(response);
            }
            return;
        }
    }

    bool ActionProcessor::handleInboundUartMessage(const UartMessage &message)
    {
        // Scaffold only: protocol-specific inbound handling will be added in a
        // dedicated pass once ACK/STATUS semantics are finalized.
        ESP_LOGI(mspTag, "Inbound UART message received (type=%u cmd=0x%04X seq=%u)",
                 message.msgType, message.commandId, message.sequence);
        return false;
    }

    bool ActionProcessor::handleSystemCommand(const actions::ActionResponse &response)
    {
        if (response.command == CMD_SYS_RPI_SHUTDOWN)
        {
            mrSerial.sendUartCommand("RPI_Shutdown", CMD_SYS_RPI_SHUTDOWN);
            mpRelayController->shutdownRpi(true);
            return true;
        }

        if (response.command == CMD_EXIT_ITEM)
        {
            ESP_LOGI(mspTag, "Sending Exit Item Message");
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

        mpRelayController->handleToggleDac(response.command == CMD_TOGGLE_DAC_ON);
        return true;
    }

    bool ActionProcessor::handleDisplayCommand(const actions::ActionResponse &response)
    {
        if (response.command != CMD_DISPLAY_OFF && response.command != CMD_DISPLAY_ON)
        {
            return false;
        }

        ESP_LOGI(mspTag, "Processing Display Toggle Command (%s)",
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
        ESP_LOGI(mspTag, "Setting Cycle Brightness Command");
        return true;
    }

    void ActionProcessor::handleHeartbeatReceived()
    {
        if (mpRpiBootManager)
        {
            mpRpiBootManager->handleHeartbeatReceived();
        }
    }

    void ActionProcessor::handleHeartbeatTimeout()
    {
        if (mpRpiBootManager)
        {
            mpRpiBootManager->handleHeartbeatTimeout();
        }
    }

    bool ActionProcessor::waitForRpiToBoot(uint32_t timeoutMs)
    {
        if (mpRpiBootManager)
        {
            return mpRpiBootManager->waitForRpiToBoot(timeoutMs);
        }
        return false;
    }

    bool ActionProcessor::waitForRpiShutdown(uint32_t timeoutMs)
    {
        if (mpRpiBootManager)
        {
            return mpRpiBootManager->waitForRpiShutdown(timeoutMs);
        }
        return false;
    }

    bool ActionProcessor::handleCommandPowerStateChange(const actions::ActionResponse &response)
    {
        return mpPowerStateTransitionHandler->handle(response);
    }
}