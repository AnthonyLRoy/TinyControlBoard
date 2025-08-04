#include "actionProcessor.hpp"

namespace controlSystem
{
    actionProcessor::actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef, spibus::SPI &spiRef) : serial(serialBusRef), relays(relaysRef), spiBus(spiRef) {}
    void actionProcessor::process(actions::actionResponse response)
    {

        switch (response.message.command_id)
        {
        case CMD_SYS_POWERON:
            relays.setRelayState(PIN_RELAY_DAC, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            relays.setRelayState(PIN_RELAY_RPI, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            relays.setRelayState(PIN_RELAY_SCREEN, true);
            break;

        default:
            break;
        }
        static u_int16_t name;
        name++;
    };
}