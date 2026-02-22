#pragma once

#include "serial.hpp"
#include "spi.hpp"
#include "relay.hpp"
#include "actionsResponse.hpp"
#include "esp_log.h"
#include "PowerStateManager.hpp"
#include <driver/gpio.h>
#include "led_Manager.hpp"
#include "RPIBootManager.hpp"
#include "RelayController.hpp"
#include "uart_protocol.hpp"
#include <memory>

namespace controlSystem
{
    class actionProcessor
    {
    public:
        struct CommandConfig
        {
            const char *logTag;
            uint32_t commandId;
        };

    public:
        actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay &relaysRef);
        void process(actions::actionResponse response);
        const char *getCommandNameForPin(uint8_t pin);
        
        // RPI boot synchronization methods
        void onHeartbeatReceived();
        void onHeartbeatTimeout();
        bool WaitForRpiToBoot(uint32_t timeoutMs = 60000);
        bool waitForPiShutdown(uint32_t timeoutMs = 60000);

    private:
        bool HandleCommandPowerStateChange(actions::actionResponse response);
        
        // Component managers
        std::unique_ptr<RPIBootManager> rpiBootManager;
        std::unique_ptr<RelayController> relayController;
        
        // References
        serialBus::Serial &serial;
        relays::StandardRelay &relays;
        
        static constexpr const char *TAG = "ActionProcessor";
    };

    extern const actionProcessor::CommandConfig commandConfigs[];
    extern const size_t NUM_COMMANDS;
}