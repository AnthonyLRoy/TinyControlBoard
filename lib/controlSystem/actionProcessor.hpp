#pragma once

#include "serial.hpp"
#include "spi.hpp"
#include "relay.hpp"
#include "actionsResponse.hpp"
#include "esp_log.h"
#include "PowerStateManager.hpp"
#include <driver/gpio.h>
#include "led_Manager.hpp"
#include "rpiBootManager.hpp"
#include "relayController.hpp"
#include "uart_protocol.hpp"
#include <memory>

namespace controlSystem
{
    class ActionProcessor
    {
    public:
        struct CommandConfig
        {
            const char *pLogTag;
            uint32_t commandId;
        };

    public:
        ActionProcessor(serialBus::Serial &rSerialBus, relays::StandardRelay &rRelays);
        void process(const actions::ActionResponse &response);
        const char *getCommandNameForPin(uint8_t pin);
        
        // RPI boot synchronization methods
        void handleHeartbeatReceived();
        void handleHeartbeatTimeout();
        bool waitForRpiToBoot(uint32_t timeoutMs = 60000);
        bool waitForRpiShutdown(uint32_t timeoutMs = 60000);

    private:
        bool handleCommandPowerStateChange(const actions::ActionResponse &response);
        
        // Component managers
        std::unique_ptr<RpiBootManager> mpRpiBootManager;
        std::unique_ptr<RelayController> mpRelayController;
        
        // References
        serialBus::Serial &mrSerial;
        relays::StandardRelay &mrRelays;
        
        static constexpr const char *mspTag = "ActionProcessor";
    };

    extern const ActionProcessor::CommandConfig commandConfigs[];
    extern const size_t NUM_COMMANDS;
}