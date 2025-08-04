#include "actionProcessor.hpp"

namespace controlSystem
{
    actionProcessor::actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef, spibus::SPI &spiRef) : serial(serialBusRef), relays(relaysRef), spiBus(spiRef) {}
    void actionProcessor::process(actions::actionResponse response)
    {

        switch (response.command)
        {
        case CMD_SYS_POWER:
            
            relays.setRelayState(PIN_RELAY_DAC, true);
            vTaskDelay(pdMS_TO_TICKS(500));  // delay for power supply to settle
            relays.setRelayState(PIN_RELAY_RPI, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            relays.setRelayState(PIN_RELAY_SCREEN, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            break;

        case CMD_NEXT_TRACK:
            UARTMessage message ;
            message.command_id = CMD_NEXT_TRACK;
            message.msg_type = MessageType::MSG_COMMAND;
            message.src_app = AppID::APP_ESP32;
            uint8_t tx_buffer[UART_PACKET_SIZE];
            serialize_message(message, tx_buffer);
            serial.send_data( (const char *)tx_buffer);
        break;


        }
        static u_int16_t name;
        name++;
    };
}