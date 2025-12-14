#include "actionProcessor.hpp"
#include "powerLed.hpp"
#include <inttypes.h>

namespace controlSystem
{
    // Constants for power state transitions
    constexpr uint32_t POWER_SETTLE_DELAY_MS = 1500;
    constexpr uint32_t SCREEN_ON_DELAY_MS = 1000;
    constexpr uint32_t LONG_PRESS_THRESHOLD_MS = 3000;

    // Array of command configurations
    const actionProcessor::CommandConfig commandConfigs[] = {
        {"POWERCOMMAND", CMD_SYS_POWER},
        {"NEXTTRACK", CMD_NEXT_TRACK},
        {"PREVTRACK", CMD_PREVIOUS_TRACK},
        {"PLAYPAUSE", CMD_PLAY_PAUSE},
        {"STOP", CMD_STOP_TRACK},
        {"SKIPFORWARD", CMD_SKIP_FORWARD},
        {"SKIPBACK", CMD_SKIP_BACK},
        {"PREVMENU", CMD_PREV_MENU_ITEM},
        {"NEXTMENU", CMD_NEXT_MENU_ITEM},
        {"ITEMSELECT", CMD_ITEM_SELECT},
        {"DISPLAYOFF", CMD_DISPLAY_OFF},
        {"METERON", CMD_TOGGLE_METER_ON},
        {"METEROFF", CMD_TOGGLE_METER_OFF},
        {"DISPLAYON", CMD_DISPLAY_ON},
        {"ROTARY", CMD_ROTARY_ACTION},

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

        if (response.command == CMD_SYS_RPI_SHUTDOWN)
        {
            serial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            relayController->ShutDownRPI(true);
            return;
        }

        if (response.command == CMD_TOGGLE_DAC_ON)
        {
            relayController->HandleToggleDac(true);
            return;
        }

        if (response.command == CMD_TOGGLE_DAC_OFF)
        {
            relayController->HandleToggleDac(false);
            return;
        }

        if (response.command == CMD_EXIT_ITEM)
        {
            ESP_LOGI(TAG, "Sending Exit Item Message");
            return;
        }

        if (response.command == CMD_ROTARY_LEFT || response.command == CMD_ROTARY_RIGHT)
        {
            ESP_LOGI(TAG, "Processing Rotary Action Command (%s)",
                     response.command == CMD_ROTARY_LEFT ? "LEFT" : "RIGHT");
            return;
        }

        // Handle commands requiring UART message
        for (size_t cmdReference = 0; cmdReference < NUM_COMMANDS; cmdReference++)
        {
            if (commandConfigs[cmdReference].commandId == response.command)
            {
                ESP_LOGI(TAG, "Sending command: %s", commandConfigs[cmdReference].logTag);
                return;
            }
        }
    }

    // Delegation methods for RPI boot management
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
        // then wait for the RPI to start , if it has not already started

        if (indicators::getPowerLed().getState() == ControlBoardPowerState::OFF ||
            indicators::getPowerLed().getState() == ControlBoardPowerState::SLEEP ||
            indicators::getPowerLed().getState() == ControlBoardPowerState::DEEPSLEEP)
        {
            // Power on sequence
            ESP_LOGI(TAG, "Initiating Power ON sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::ON);

            relayController->setRelayWithDelay(PIN_RELAY_SCREEN, true, SCREEN_ON_DELAY_MS);
            relayController->setRelayWithDelay(PIN_RELAY_DAC, true, POWER_SETTLE_DELAY_MS);
            relayController->setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE, true, POWER_SETTLE_DELAY_MS);
            relayController->setRelayWithDelay(PIN_RELAY_RPI, true, SCREEN_ON_DELAY_MS);

            bool booted = WaitForRpiToBoot(60000);
            return true && booted;
        }

        // switching off sequence    short press = sleep
        // 1) sleep keeps switches of power to the RPI and the Screenn but  leaves the power to the DAC and pre amplifiers
        // 2)  (press for 3 seconds or more) switches off the power to the screen ,
        // the RPI and the DACS and output preamps , but leves the 3.3v to the reclock-crystal boards //long press = deep sleep
        ESP_LOGI(TAG, "Release Time MS: %" PRIu16 "", response.releaseTimeMilliSecs );
        if (indicators::getPowerLed().getState() == ControlBoardPowerState::ON && response.releaseTimeMilliSecs < LONG_PRESS_THRESHOLD_MS)
        {
            // Sleep sequence
            ESP_LOGI(TAG, "Initiating Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);

            serial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            waitForPiShutdown(60000);
            relayController->ShutDownRPI(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            relayController->ShutDownScreen(false);
            vTaskDelay(pdMS_TO_TICKS(5000));
            indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);
            return true;
        }
        // Deep sleep if long press exceeds threshold
        if (indicators::getPowerLed().getState() == ControlBoardPowerState::ON && response.releaseTimeMilliSecs > LONG_PRESS_THRESHOLD_MS)
        {
            ESP_LOGI(TAG, "Initiating Deep Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);
            serial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            waitForPiShutdown(60000);
            relayController->ShutDownRPI(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            relayController->ShutDownScreen(false);
            relayController->setRelayWithDelay(PIN_RELAY_DAC, false, 0);
            relayController->setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE, false, 0);
            indicators::getPowerLed().setState(ControlBoardPowerState::DEEPSLEEP);
        }
        return true;
    }
}
