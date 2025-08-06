#include "actionProcessor.hpp"

namespace controlSystem
{
    actionProcessor::actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef, spibus::SPI &spiRef) : serial(serialBusRef), relays(relaysRef), spiBus(spiRef) {}
    void actionProcessor::process(actions::actionResponse response)
    {

        UARTMessage message;
        switch (response.command)
        {
        case CMD_NEXT_TRACK:
            ESP_LOGI("NEXTTRACK", "Sending next Track Message");
            message.command_id = CMD_NEXT_TRACK;
            break;

        case CMD_PREVIOUS_TRACK:
            ESP_LOGI("PREVTRACK", "Sending previous Track Message");
            message.command_id = CMD_PREVIOUS_TRACK;
            break;
        case CMD_PLAY_PAUSE:
            ESP_LOGI("PLAYPAUSE", "Sending Play/Pause Message");
            message.command_id = CMD_PLAY_PAUSE;
            break;
        case CMD_STOP_TRACK:
            ESP_LOGI("STOP", "Sending Stop Track Message");
            message.command_id = CMD_STOP_TRACK;
            break;
        case CMD_SKIP_FORWARD:
            ESP_LOGI("SKIPFORWARD", "Sending Skip Forward Message");
            message.command_id = CMD_SKIP_FORWARD;
            break;
        case CMD_SKIP_BACK:
            ESP_LOGI("SKIPBACK", "Sending Skip Back Message");
            message.command_id = CMD_SKIP_BACK;
            break;
        case CMD_PREV_MENU_ITEM:
            ESP_LOGI("PREVMENU", "Sending Previous Menu Item Message");
            message.command_id = CMD_PREV_MENU_ITEM;
            break;
        case CMD_NEXT_MENU_ITEM:
            ESP_LOGI("NEXTMENU", "Sending Next Menu Item Message");
            message.command_id = CMD_NEXT_MENU_ITEM;
            break;
        case CMD_ITEM_SELECT:
            ESP_LOGI("ITEMSELECT", "Sending Item Select Message");
            message.command_id = CMD_ITEM_SELECT;
            break;
        case CMD_EXIT_ITEM:
            ESP_LOGI("EXITITEM", "Sending Exit Item Message");
            break;
        case CMD_DISPLAY_OFF:
            ESP_LOGI("DISPLAYOFF", "Sending Display Off Message");
            message.command_id = CMD_DISPLAY_OFF;
            break;
        case CMD_TOGGLE_METER_ON:
            ESP_LOGI("METERON", "Sending Meter On Message");
            message.command_id = CMD_TOGGLE_METER_ON;
            break;
        case CMD_TOGGLE_METER_OFF:
            ESP_LOGI("METEROFF", "Sending Meter Off Message");
            message.command_id = CMD_TOGGLE_METER_OFF;
            break;
        case CMD_DISPLAY_ON:
            ESP_LOGI("DISPLAYON", "Sending Display On Message");
            message.command_id = CMD_DISPLAY_ON;
            break;
        case CMD_TOGGLE_DAC_ON:
            HandleToggleDac(true);
            break;
        case CMD_TOGGLE_DAC_OFF:
            HandleToggleDac(false);
            break;
        case CMD_ROTARY_LEFT:
            ESP_LOGI("ROTARYLEFT", "Sending Rotary Left Message");
            message.command_id = CMD_ROTARY_LEFT;
            break;
        case CMD_ROTARY_RIGHT:
            ESP_LOGI("ROTARYRIGHT", "Sending Rotary Right Message");
            message.command_id = CMD_ROTARY_RIGHT;
            break;
        case CMD_NO_ACTION:
            // No action needed, just return
            return;
        case CMD_SYS_RPI_SHUTDOWN:
            ShutDownRPI(true);
            return;

        case CMD_SYS_POWER:
            HandleCommandPowerStateChange(response);
            return;

        default:
            return;
        }

        //send the message for all items that require it
        uint8_t tx_buffer[UART_PACKET_SIZE];    
        serialize_message(message, tx_buffer);
        if (!serial.send_data((const char *)tx_buffer)) {
            ESP_LOGE("UART", "Failed to send data");
            return;
        }
    };

    bool actionProcessor::HandleToggleDac(bool state)
    {
        relays.setRelayState(PIN_RELAY_DAC, state);
        return true;
    }

    bool actionProcessor::HandleCommandPowerStateChange(actions::actionResponse resposne)
    {
        auto &stateMgr = PowerStateManager::instance();

        if (stateMgr.getPowerState() == ControlBoardPowerState::OFF)
        {
            // switch on the relays
            relays.setRelayState(PIN_RELAY_DAC, true);          // dav 5v and 3.3
            vTaskDelay(pdMS_TO_TICKS(1500));                    // delay for power supply to settle
            relays.setRelayState(PIN_RELAY_OUTPUT_STAGE, true); // dav 5v and 3.3
            vTaskDelay(pdMS_TO_TICKS(1500));                    // delay for power supply to settle

            relays.setRelayState(PIN_RELAY_RPI, true); // switch on the RPI
            vTaskDelay(pdMS_TO_TICKS(1000));
            relays.setRelayState(PIN_RELAY_SCREEN, true); // swithc on the screen
            vTaskDelay(pdMS_TO_TICKS(1000));
            return true;
        }

        if (stateMgr.getPowerState() == ControlBoardPowerState::ON && resposne.releaseTimeMilliSecs > 3000)
        {
            stateMgr.setPowerState(ControlBoardPowerState::GOING_TO_SLEEP);
            // sleep switch off the screen and  the RPI
            // Fuck this just shut down the RPI this bitch is burning too much power
            spiBus.send(0xAAAA);

            UARTMessage message;
            message.command_id = CMD_STOP_TRACK;
            uint8_t tx_buffer[UART_PACKET_SIZE];
            serialize_message(message, tx_buffer);
            serial.send_data((const char *)tx_buffer);

            spiBus.send(0xAAA0);

           bool rpiState = ShutDownRPI(true);
            spiBus.send(0xAA00);

            bool screenState = ShutDownScreen(false);
            spiBus.send(0xA000);

            vTaskDelay(pdMS_TO_TICKS(500));

            spiBus.send(0x0000);
            stateMgr.setPowerState(ControlBoardPowerState::SLEEP);

            // Go into deep sleep
            if (resposne.releaseTimeMilliSecs > 8000)
            {

                relays.setRelayState(PIN_RELAY_DAC, false);
                relays.setRelayState(PIN_RELAY_OUTPUT_STAGE, false);
            };
            return false;
        }
        return true;
    }

    bool actionProcessor::ShutDownRPI(bool wait)
    {
        // send message to shutdown RPI and wait
        UARTMessage message;
        message.command_id = CMD_SYS_RPI_SHUTDOWN;
        uint8_t tx_buffer[UART_PACKET_SIZE];
        serialize_message(message, tx_buffer);
        serial.send_data((const char *)tx_buffer);

        // wait here for a certain period of time then swith of RELAY anyway

        relays::StandardRelay::setRelayState(PIN_RELAY_RPI, false);

        return false;
    }

    bool actionProcessor::ShutDownScreen(bool wait)
    {
        relays::StandardRelay::setRelayState(PIN_RELAY_SCREEN, false);
        return true;
    }

}