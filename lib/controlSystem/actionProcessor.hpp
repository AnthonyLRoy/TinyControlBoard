#pragma once

#include "serial.hpp"
#include "spi.hpp"
#include "relay.hpp"
#include "actionsResponse.hpp"
#include "esp_log.h"
#include "PowerStateManager.hpp"

namespace controlSystem
{
    class actionProcessor
    {
    private:
        /* data */
    public:
        actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef, spibus::SPI &spiRef);
        void process(actions::actionResponse response);

    private:
        bool HandleCommandPowerStateChange(actions::actionResponse resposne);
        bool HandleToggleDac(bool state);
        bool ShutDownRPI(bool wait);
        bool ShutDownScreen(bool wait);

        serialBus::Serial &serial;
        relays::StandardRelay &relays;
        spibus::SPI &spiBus;
    };
}