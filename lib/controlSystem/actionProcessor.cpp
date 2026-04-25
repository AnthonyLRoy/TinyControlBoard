#include "actionProcessor.hpp"
#include "powerLed.hpp"
#include <inttypes.h>

namespace controlSystem
{
    ActionProcessor::ActionProcessor(serialBus::Serial &rSerialBus, relays::StandardRelay &rRelays)
        : mrSerial(rSerialBus), mrRelays(rRelays)
    {
        // Create component managers
        mpSerialUartCommandSink = std::make_unique<SerialUartCommandSink>(mrSerial);
        mpActionUartDispatcher = std::make_unique<ActionUartDispatcher>(*mpSerialUartCommandSink);
        mpRpiBootManager = std::make_unique<RpiBootManager>();
        mpRelayController = std::make_unique<RelayController>(mrSerial, mrRelays);
        mpPowerStateTransitionHandler = std::make_unique<PowerStateTransitionHandler>(
            mrSerial,
            *mpRelayController,
            *mpRpiBootManager);
    }

    void ActionProcessor::process(const actions::ActionResponse &response)
    {
        ESP_LOGI(mspTag, "Action Processor received command: 0x%04X", response.command);
        
        if (response.command == CMD_NO_ACTION)
        {
            return;
        }

        if (response.command == CMD_SYS_POWER)
        {
            ESP_LOGI(mspTag, "Processing Power State Change Command");
            handleCommandPowerStateChange(response);
            return;
        }

        const auto powerState = indicators::getPowerLed().getState();
        if (powerState != ControlBoardPowerState::ON)
        {
            ESP_LOGI(mspTag, "Ignoring command %u as system is not ON", response.command);
            return;
        }

        if (handleSystemCommand(response) ||
            handleRelayCommand(response) ||
            handleDisplayCommand(response) ||
            handleBrightnessCommand(response) ||
            (mpActionUartDispatcher && mpActionUartDispatcher->handle(response)))
        {
            return;
        }
    }

    bool ActionProcessor::handleSystemCommand(const actions::ActionResponse &response)
    {
        // todo remove as handled in power state change
        if (response.command == CMD_SYS_RPI_SHUTDOWN)
        {
            mrSerial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            mpRelayController->shutdownRpi(true);
            return true;
        }

        // todo no longer needed
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

    // handler methods for RPI boot management
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
