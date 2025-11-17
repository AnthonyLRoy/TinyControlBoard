#include "actionProcessor.hpp"

namespace controlSystem
{
    // Constants for clarity
    constexpr uint32_t POWER_SETTLE_DELAY_MS = 1500;
    constexpr uint32_t SCREEN_ON_DELAY_MS = 1000;
    constexpr uint32_t LONG_PRESS_THRESHOLD_MS = 3000;
    constexpr uint32_t DEEP_SLEEP_THRESHOLD_MS = 8000;

    // SPI command codes to show the Lights pattern to the user to show Shutting down taking place
    constexpr uint16_t SPI_INIT_SHUTDOWN = 0xAAAA;
    constexpr uint16_t SPI_STOP_TRACK_SENT = 0xAAA0;
    constexpr uint16_t SPI_RPI_SHUTDOWN_SENT = 0xAA00;
    constexpr uint16_t SPI_SCREEN_SHUTDOWN_SENT = 0xA000;
    constexpr uint16_t SPI_ALL_OFF = 0x0000;

    // Array of command configurations
    const actionProcessor::CommandConfig commandConfigs[] = {
        {"NEXTTRACK", CMD_NEXT_TRACK},          //pin 1
        {"PREVTRACK", CMD_PREVIOUS_TRACK},      //pin 2
        {"PLAYPAUSE", CMD_PLAY_PAUSE},          //pin 3
        {"STOP", CMD_STOP_TRACK},               //pin 4
        {"SKIPFORWARD", CMD_SKIP_FORWARD},      //pin 5
        {"SKIPBACK", CMD_SKIP_BACK},            //pin 6
        {"PREVMENU", CMD_PREV_MENU_ITEM},       //pin 7
        {"NEXTMENU", CMD_NEXT_MENU_ITEM},       //pin 8
        {"ITEMSELECT", CMD_ITEM_SELECT},        //pin 9
        {"DISPLAYOFF", CMD_DISPLAY_OFF},        //pin 10   
        {"METERON", CMD_TOGGLE_METER_ON},       //pin 11
        {"METEROFF", CMD_TOGGLE_METER_OFF},     //pin 12
        {"DISPLAYON", CMD_DISPLAY_ON},          //pin 13
        {"ROTARYLEFT", CMD_ROTARY_LEFT},        //pin 14
        {"ROTARYRIGHT", CMD_ROTARY_RIGHT},      //pin 15
        {"POWERCOMMAND", CMD_SYS_POWER}         //pin 16
    };

    const size_t NUM_COMMANDS = sizeof(commandConfigs) / sizeof(commandConfigs[0]);

    actionProcessor::actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef, spibus::SPI &spiRef)
        : serial(serialBusRef), relays(relaysRef), spiBus(spiRef) {}

    void actionProcessor::process(actions::actionResponse response)
    {
        auto &stateMgr = PowerStateManager::instance();

        if (response.command == CMD_NO_ACTION)
        {
            return;
        }
        
        if (response.command == CMD_SYS_POWER)
        {
            HandleCommandPowerStateChange(response);
            return;
        }
 
        if(stateMgr.getPowerState() != ControlBoardPowerState::ON)
        {
            ESP_LOGI("ActionProcessor", "Ignoring command %u as system is not ON", response.command);
            return;
        }       
         
        if (response.command == CMD_SYS_RPI_SHUTDOWN)
        {
            ShutDownRPI(true);
            return;
        }

        
        if (response.command == CMD_TOGGLE_DAC_ON)
        {
            HandleToggleDac(true);
            return;
        }
        if (response.command == CMD_TOGGLE_DAC_OFF)
        {
            HandleToggleDac(false);
            return;
        }
        if (response.command == CMD_EXIT_ITEM)
        {
            ESP_LOGI("EXITITEM", "Sending Exit Item Message");
            return;
        }

        // Handle commands requiring UART message
        for (size_t cmdReference = 0; cmdReference < NUM_COMMANDS; ++cmdReference)
        {
            if (commandConfigs[cmdReference].commandId == response.command)
            {
                sendUartCommand(commandConfigs[cmdReference].logTag, commandConfigs[cmdReference].commandId);
                return;
            }
        }
    }

    void actionProcessor::sendUartCommand(const char *logTag, uint32_t commandId)
    {
        UARTMessage message;
        message.command_id = commandId;
        uint8_t tx_buffer[UART_PACKET_SIZE];
        serialize_message(message, tx_buffer);
        ESP_LOGI(logTag, "Sending %s Message", logTag);
        if (!serial.send_data(tx_buffer, UART_PACKET_SIZE))
            ESP_LOGI(logTag, "Failed to send %s message", logTag);
        else
            ESP_LOGI(logTag, "%s message sent successfully", logTag);
    }

    bool actionProcessor::HandleToggleDac(bool state)
    {
        relays.setRelayState(PIN_RELAY_DAC, state);
        return true;
    }

    void actionProcessor::setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs)
    {
        ESP_LOGI("RelayControl", "Setting relay %d to %s with delay %lu ms", pin, state ? "ON" : "OFF", delayMs);
        relays.setRelayState(pin, state);
        if (delayMs > 0)
        {
            vTaskDelay(pdMS_TO_TICKS(delayMs));
        }
    }

    bool actionProcessor::HandleCommandPowerStateChange(actions::actionResponse response)
    {
        auto &stateMgr = PowerStateManager::instance();

        if (stateMgr.getPowerState() == ControlBoardPowerState::OFF)
        {
            //prevent multiple power on commands
            stateMgr.setPowerState(ControlBoardPowerState::ON);
            // Power on sequence
            setRelayWithDelay(PIN_RELAY_DAC, true, POWER_SETTLE_DELAY_MS);
            setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE, true, POWER_SETTLE_DELAY_MS);
            setRelayWithDelay(PIN_RELAY_RPI, true, SCREEN_ON_DELAY_MS);
            setRelayWithDelay(PIN_RELAY_SCREEN, true, SCREEN_ON_DELAY_MS);

            return true;
        }

        if (stateMgr.getPowerState() == ControlBoardPowerState::ON && response.releaseTimeMilliSecs > LONG_PRESS_THRESHOLD_MS)
        {
            // Power off or sleep sequence
            stateMgr.setPowerState(ControlBoardPowerState::GOING_TO_SLEEP);
            spiBus.send(SPI_INIT_SHUTDOWN);

            sendUartCommand("STOP", CMD_STOP_TRACK);
            spiBus.send(SPI_STOP_TRACK_SENT);

            ShutDownRPI(true);
            spiBus.send(SPI_RPI_SHUTDOWN_SENT);

            ShutDownScreen(false);
            spiBus.send(SPI_SCREEN_SHUTDOWN_SENT);

            vTaskDelay(pdMS_TO_TICKS(500));
            spiBus.send(SPI_ALL_OFF);
            stateMgr.setPowerState(ControlBoardPowerState::SLEEP);

            // Deep sleep if long press exceeds threshold
            if (response.releaseTimeMilliSecs > DEEP_SLEEP_THRESHOLD_MS)
            {
                setRelayWithDelay(PIN_RELAY_DAC, false, 0);
                setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE, false, 0);
            }
            return false;
        }
        return true;
    }

    bool actionProcessor::ShutDownRPI(bool wait)
    {
        sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
        relays.setRelayState(PIN_RELAY_RPI, false);
        return false;
    }

    bool actionProcessor::ShutDownScreen(bool wait)
    {
        relays.setRelayState(PIN_RELAY_SCREEN, false);
        return true;
    }


}