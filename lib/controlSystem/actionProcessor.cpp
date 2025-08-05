#include "actionProcessor.hpp"

namespace controlSystem
{
    actionProcessor::actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef, spibus::SPI &spiRef) : serial(serialBusRef), relays(relaysRef), spiBus(spiRef) {}
    void actionProcessor::process(actions::actionResponse response)
    {

        switch (response.command)
        {

        case CMD_STOP_TRACK:
            break;

        case CMD_SYS_POWER:
            HandleCommandPowerStateChange(response);

            break;

        case CMD_NEXT_TRACK:
        {
            ESP_LOGI("NEXTTRACK", "Sending next Track Message");
            UARTMessage message;
            message.command_id = CMD_NEXT_TRACK;
            message.msg_type = MessageType::MSG_COMMAND;
            message.src_app = AppID::APP_ESP32;
            uint8_t tx_buffer[UART_PACKET_SIZE];
            serialize_message(message, tx_buffer);
            serial.send_data((const char *)tx_buffer);
            break;
        }
        default:
            break;
        }

    };

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

            bool ShutDownRPI(true);
            spiBus.send(0xAA00);

            bool ShutDownScreen(false);
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