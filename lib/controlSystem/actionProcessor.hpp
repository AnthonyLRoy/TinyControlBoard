#pragma once

#include "serial.hpp"
#include "spi.hpp"
#include "relay.hpp"
#include "actionsResponse.hpp"
#include "esp_log.h"
#include "PowerStateManager.hpp"
#include <driver/gpio.h> // Added for gpio_num_t
#include "led_Manager.hpp"

namespace controlSystem
{

    class actionProcessor
    {
    public:
        struct CommandConfig
        {
            const char *logTag;
            uint32_t commandId; // Using uint32_t as assumed type for CMD_* constants because they of version of c++ i think
        };

    public:
        actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef, spibus::SPI &spiRef);
        void process(actions::actionResponse response);
        const char *getCommandNameForPin(uint8_t pin);

    private:
        bool HandleCommandPowerStateChange(actions::actionResponse response);
        bool HandleToggleDac(bool state);
        bool ShutDownRPI(bool wait);
        bool ShutDownScreen(bool wait);
        void sendUartCommand(const char *logTag, UARTMessage message);
        void sendUartCommand(const char *logTag, uint32_t commandId);
        void setRelayWithDelay(gpio_num_t pin, bool state, uint32_t delayMs); // Changed uint8_t to gpio_num_t
        serialBus::Serial &serial;
        relays::StandardRelay &relays;
        spibus::SPI &spiBus;
    };

    extern const actionProcessor::CommandConfig commandConfigs[];
    extern const size_t NUM_COMMANDS;

}