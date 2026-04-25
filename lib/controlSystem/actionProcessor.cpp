#include "actionProcessor.hpp"
#include "powerLed.hpp"
#include <inttypes.h>

namespace controlSystem
{
    // Constants for power state transitions
    constexpr uint32_t POWER_SETTLE_DELAY_MS = 1500;
    constexpr uint32_t SCREEN_ON_DELAY_MS = 1000;
    constexpr uint32_t LONG_PRESS_THRESHOLD_MS = 3000;
    constexpr uint32_t RPI_BOOT_TIMEOUT_MS = 60000;
    constexpr uint32_t RPI_SHUTDOWN_TIMEOUT_MS = 60000;

    // Array of command configurations
    const ActionProcessor::CommandConfig commandConfigs[] = {
        {"POWERCOMMAND", CMD_SYS_POWER},
        {"NEXTTRACK", CMD_NEXT_TRACK},
        {"PREVTRACK", CMD_PREVIOUS_TRACK},
        {"PLAYPAUSE", CMD_PLAY_PAUSE},
        {"STOP", CMD_STOP_TRACK},
        {"SKIPFORWARD", CMD_SKIP_FORWARD},
        {"SKIPBACK", CMD_SKIP_BACK},
        {"COVER", CMD_TOGGLE_COVER_VIEW},
        {"NEXTMENU", CMD_NEXT_MENU_ITEM},
        {"ITEMSELECT", CMD_ITEM_SELECT},
        {"DISPLAYOFF", CMD_DISPLAY_OFF},
        {"METERON", CMD_TOGGLE_METER_ON},
        {"METEROFF", CMD_TOGGLE_METER_OFF},
        {"DISPLAYON", CMD_DISPLAY_ON},
        {"ROTARY", CMD_ROTARY_ACTION},
        {"TOGGLEDAC", CMD_TOGGLE_DAC},
        {"TOGGLEDISPLAY", CMD_TOGGLE_DISPLAY},
        {"TOGGLEMETER", CMD_TOGGLE_METER},
        {"CYCLEBRIGHTNESS", CMD_CYCLE_BRIGHTNESS}

    };

    const size_t NUM_COMMANDS = sizeof(commandConfigs) / sizeof(commandConfigs[0]);

    ActionProcessor::ActionProcessor(serialBus::Serial &rSerialBus, relays::StandardRelay &rRelays)
        : mrSerial(rSerialBus), mrRelays(rRelays)
    {
        // Create component managers
        mpRpiBootManager = std::make_unique<RpiBootManager>();
        mpRelayController = std::make_unique<RelayController>(mrSerial, mrRelays);
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

        // todo remove as handled in power state change
        if (response.command == CMD_SYS_RPI_SHUTDOWN)
        {
            mrSerial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            mpRelayController->shutdownRpi(true);
            return;
        }
        // todo no longer needed
        if (response.command == CMD_EXIT_ITEM)
        {
            ESP_LOGI(mspTag, "Sending Exit Item Message");
            return;
        }

        if (response.command == CMD_TOGGLE_DAC_ON || response.command == CMD_TOGGLE_DAC_OFF)
        {
            mpRelayController->handleToggleDac(response.command == CMD_TOGGLE_DAC_ON);
            return;
        }
        if (response.command == CMD_COVER_VIEW_ON || response.command == CMD_COVER_VIEW_OFF)
        {
            ESP_LOGI(mspTag, "Processing Cover View Toggle Command (%s)",
                     response.command == CMD_COVER_VIEW_ON ? "ON" : "OFF");

            UartMessage message;
            message.commandId = CMD_TOGGLE_COVER_VIEW;
            message.params[0] = (response.command == CMD_COVER_VIEW_ON) ? 1 : 0;
            mrSerial.sendUartMessage("COVERVIEW", message);
            return;
        }   

        if (response.command == CMD_DISPLAY_OFF || response.command == CMD_DISPLAY_ON)
        {
            ESP_LOGI(mspTag, "Processing Display Toggle Command (%s)",
                     response.command == CMD_DISPLAY_ON ? "ON" : "OFF");

            indicators::getMonitorBrightnessController().setBlanked(response.command == CMD_DISPLAY_OFF);
            return;
        }

        if (response.command == CMD_TOGGLE_METER_ON || response.command == CMD_TOGGLE_METER_OFF)
        {
            ESP_LOGI(mspTag, "Processing Meter Toggle Command (%s)",
                     response.command == CMD_TOGGLE_METER_ON ? "ON" : "OFF");
            UartMessage message;
            message.commandId = CMD_TOGGLE_METER;
            message.params[0] = (response.command == CMD_TOGGLE_METER_ON) ? 1 : 0;
            mrSerial.sendUartMessage("METER", message);
            return;
        }

        if (response.command == CMD_ROTARY_ACTION)
        {
            UartMessage message;
            message.commandId = response.command;
            message.params[0] = (response.parameters[0]);
            mrSerial.sendUartMessage("ROTARY", message);
            ESP_LOGI(mspTag, "Processing Rotary Action Command (%s)",
                     response.parameters[0] == 0 ? "LEFT" : "RIGHT");
            return;
        }

        if (response.command == CMD_CYCLE_BRIGHTNESS)
        {
            indicators::getMonitorBrightnessController().cycleBrightness();
            
            ESP_LOGI(mspTag, "Setting Cycle Brightness Command");
            return;
        }

        // Handle simple commands requiring UART message
        for (size_t cmdReference = 0; cmdReference < NUM_COMMANDS; cmdReference++)
        {
            if (commandConfigs[cmdReference].commandId == response.command)
            {
                mrSerial.sendUartCommand(commandConfigs[cmdReference].pLogTag, response.command);
                ESP_LOGI(mspTag, "Sending command: %s", commandConfigs[cmdReference].pLogTag);
                return;
            }
        }
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
        // Handle power button action based on current power state and press duration.
        // Sequences are relay-driven with delays and optional RPI boot/shutdown waits.
        const auto powerState = indicators::getPowerLed().getState();
        if (powerState == ControlBoardPowerState::OFF ||
            powerState == ControlBoardPowerState::SLEEP ||
            powerState == ControlBoardPowerState::DEEPSLEEP)
        {
            indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);
            // Power on sequence
            ESP_LOGI(mspTag, "Initiating Power ON sequence");

            mpRelayController->setRelayWithDelay(PIN_RELAY_SCREEN_POWER, true, SCREEN_ON_DELAY_MS);
            mpRelayController->setRelayWithDelay(PIN_RELAY_DAC_POWER, true, POWER_SETTLE_DELAY_MS);
            mpRelayController->setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, true, POWER_SETTLE_DELAY_MS);
            mpRelayController->setRelayWithDelay(PIN_RELAY_RPI_POWER, true, SCREEN_ON_DELAY_MS);

            bool booted = waitForRpiToBoot(RPI_BOOT_TIMEOUT_MS);
            indicators::getPowerLed().setState(ControlBoardPowerState::ON);
            indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Active);
            return booted;
        }

        // Power-down paths from ON:
        // - Short press: sleep keeps DAC/output stage powered but turns off RPI and screen.
        // - Long press: deep sleep removes RPI/screen/DAC/output stage power while keeping 3.3V rails.
        ESP_LOGI(mspTag, "Release Time MS: %" PRIu16 "", response.releaseTimeMillis);
        if (powerState == ControlBoardPowerState::ON &&
            response.releaseTimeMillis < LONG_PRESS_THRESHOLD_MS)
        {
            // Sleep sequence
            ESP_LOGI(mspTag, "Initiating Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);

            mrSerial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            waitForRpiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            mpRelayController->shutdownRpi(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            mpRelayController->shutdownScreen(false);
            vTaskDelay(pdMS_TO_TICKS(5000));
            indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);
            indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::sleeping);
            return true;
        }
        // Deep sleep if long press exceeds threshold
        if (powerState == ControlBoardPowerState::ON &&
            response.releaseTimeMillis >= LONG_PRESS_THRESHOLD_MS)
        {
            ESP_LOGI(mspTag, "Initiating Deep Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);
            mrSerial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            waitForRpiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            mpRelayController->shutdownRpi(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            mpRelayController->shutdownScreen(false);
            mpRelayController->setRelayWithDelay(PIN_RELAY_DAC_POWER, false, 0);
            mpRelayController->setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, false, 0);
            indicators::getPowerLed().setState(ControlBoardPowerState::DEEPSLEEP);
            indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::sleeping);
        }
        return true;
    }
}
