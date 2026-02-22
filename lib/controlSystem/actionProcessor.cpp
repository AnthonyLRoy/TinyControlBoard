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
    const actionProcessor::CommandConfig commandConfigs[] = {
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

    actionProcessor::actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef)
        : serial(serialBusRef), relays(relaysRef)
    {
        // Create component managers
        rpiBootManager = std::make_unique<RPIBootManager>();
        relayController = std::make_unique<RelayController>(serial, relays);
    }

    void actionProcessor::process(actions::actionResponse response)
    {
        ESP_LOGI(TAG, "Action Processor received command: 0x%04X", response.command);
        
        if (response.command == CMD_NO_ACTION)
        {
            return;
        }

        if (response.command == CMD_SYS_POWER)
        {
            ESP_LOGI(TAG, "Processing Power State Change Command");
            HandleCommandPowerStateChange(response);
            return;
        }

        if (indicators::getPowerLed().getState() != ControlBoardPowerState::ON)
        {
            ESP_LOGI(TAG, "Ignoring command %u as system is not ON", response.command);
            return;
        }

        // todo remove as handled in power state change
        if (response.command == CMD_SYS_RPI_SHUTDOWN)
        {
            serial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            relayController->ShutDownRPI(true);
            return;
        }
        // todo no longer needed
        if (response.command == CMD_EXIT_ITEM)
        {
            ESP_LOGI(TAG, "Sending Exit Item Message");
            return;
        }

        if (response.command == CMD_TOGGLE_DAC_ON || response.command == CMD_TOGGLE_DAC_OFF)
        {
            relayController->HandleToggleDac(response.command == CMD_TOGGLE_DAC_ON);
            return;
        }
        if (response.command == CMD_COVER_VIEW_ON || response.command == CMD_COVER_VIEW_OFF)
        {
            ESP_LOGI(TAG, "Processing Cover View Toggle Command (%s)",
                     response.command == CMD_COVER_VIEW_ON ? "ON" : "OFF");

            UARTMessage message;
            message.command_id = CMD_TOGGLE_COVER_VIEW;
            message.params[0] = (response.command == CMD_COVER_VIEW_ON) ? 1 : 0;
            serial.sendUartMessage("COVERVIEW", message);
            return;
        }   

        if (response.command == CMD_DISPLAY_OFF || response.command == CMD_DISPLAY_ON)
        {
            ESP_LOGI(TAG, "Processing Display Toggle Command (%s)",
                     response.command == CMD_DISPLAY_ON ? "ON" : "OFF");

            UARTMessage message;
            message.command_id = CMD_TOGGLE_DISPLAY;
            message.params[0] = (response.command == CMD_DISPLAY_ON) ? 1 : 0;
            serial.sendUartMessage("DISPLAY", message);
            return;
        }

        if (response.command == CMD_TOGGLE_METER_ON || response.command == CMD_TOGGLE_METER_OFF)
        {
            ESP_LOGI(TAG, "Processing Meter Toggle Command (%s)",
                     response.command == CMD_TOGGLE_METER_ON ? "ON" : "OFF");
            UARTMessage message;
            message.command_id = CMD_TOGGLE_METER;
            message.params[0] = (response.command == CMD_TOGGLE_METER_ON) ? 1 : 0;
            serial.sendUartMessage("METER", message);
            return;
        }

        if (response.command == CMD_ROTARY_ACTION)
        {
            UARTMessage message;
            message.command_id = response.command;
            message.params[0] = (response.parameters[0]);
            serial.sendUartMessage("ROTARY", message);
            ESP_LOGI(TAG, "Processing Rotary Action Command (%s)",
                     response.command == CMD_ROTARY_LEFT ? "LEFT" : "RIGHT");
            return;
        }

        // Handle simple commands requiring UART message
        for (size_t cmdReference = 0; cmdReference < NUM_COMMANDS; cmdReference++)
        {
            if (commandConfigs[cmdReference].commandId == response.command)
            {
                serial.sendUartCommand(commandConfigs[cmdReference].logTag, response.command);
                ESP_LOGI(TAG, "Sending command: %s", commandConfigs[cmdReference].logTag);
                return;
            }
        }
    }

    // handler methods for RPI boot management
    void actionProcessor::onHeartbeatReceived()
    {
        if (rpiBootManager)
        {
            rpiBootManager->onHeartbeatReceived();
        }
    }

    void actionProcessor::onHeartbeatTimeout()
    {
        if (rpiBootManager)
        {
            rpiBootManager->onHeartbeatTimeout();
        }
    }

    bool actionProcessor::WaitForRpiToBoot(uint32_t timeoutMs)
    {
        if (rpiBootManager)
        {
            return rpiBootManager->WaitForRpiToBoot(timeoutMs);
        }
        return false;
    }

    bool actionProcessor::waitForPiShutdown(uint32_t timeoutMs)
    {
        if (rpiBootManager)
        {
            return rpiBootManager->waitForPiShutdown(timeoutMs);
        }
        return false;
    }

    bool actionProcessor::HandleCommandPowerStateChange(actions::actionResponse response)
    {

        // if power is OFF or SLEEP, turn ON
        // we do this by switching on all necessary relays with delays
        // then wait for the RPI to start  if it has not already started

        if (indicators::getPowerLed().getState() == ControlBoardPowerState::OFF ||
            indicators::getPowerLed().getState() == ControlBoardPowerState::SLEEP ||
            indicators::getPowerLed().getState() == ControlBoardPowerState::DEEPSLEEP)
        {
            indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);
            // Power on sequence
            ESP_LOGI(TAG, "Initiating Power ON sequence");

            relayController->setRelayWithDelay(PIN_RELAY_SCREEN_POWER, true, SCREEN_ON_DELAY_MS);
            relayController->setRelayWithDelay(PIN_RELAY_DAC_POWER, true, POWER_SETTLE_DELAY_MS);
            relayController->setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, true, POWER_SETTLE_DELAY_MS);
            relayController->setRelayWithDelay(PIN_RELAY_RPI_POWER, true, SCREEN_ON_DELAY_MS);

            bool booted = WaitForRpiToBoot(RPI_BOOT_TIMEOUT_MS);
            indicators::getPowerLed().setState(ControlBoardPowerState::ON);
            indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::Active);
            return true && booted;
        }

        // switching off sequence    short press = sleep
        // 1) sleep keeps switches of power to the RPI and the Screenn but  leaves the power to the DAC and pre amplifiers
        // 2)  (press for 3 seconds or more) switches off the power to the screen ,
        // the RPI and the DACS and output preamps , but leves the 3.3v to the reclock-crystal boards //long press = deep sleep
        ESP_LOGI(TAG, "Release Time MS: %" PRIu16 "", response.releaseTimeMilliSecs);
        if (indicators::getPowerLed().getState() == ControlBoardPowerState::ON && response.releaseTimeMilliSecs < LONG_PRESS_THRESHOLD_MS)
        {
            // Sleep sequence
            ESP_LOGI(TAG, "Initiating Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);

            serial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            waitForPiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            relayController->ShutDownRPI(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            relayController->ShutDownScreen(false);
            vTaskDelay(pdMS_TO_TICKS(5000));
            indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);
            indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::sleeping);
            return true;
        }
        // Deep sleep if long press exceeds threshold
        if (indicators::getPowerLed().getState() == ControlBoardPowerState::ON && response.releaseTimeMilliSecs > LONG_PRESS_THRESHOLD_MS)
        {
            ESP_LOGI(TAG, "Initiating Deep Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);
            serial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            waitForPiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            relayController->ShutDownRPI(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            relayController->ShutDownScreen(false);
            relayController->setRelayWithDelay(PIN_RELAY_DAC_POWER, false, 0);
            relayController->setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, false, 0);
            indicators::getPowerLed().setState(ControlBoardPowerState::DEEPSLEEP);
            indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::sleeping);
        }
        return true;
    }
}
