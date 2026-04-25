#pragma once

#include "serial.hpp"
#include "spi.hpp"
#include "relay.hpp"
#include "actionsResponse.hpp"
#include "ActionUartDispatcher.hpp"
#include "esp_log.h"
#include "power/powerState.hpp"
#include <driver/gpio.h>
#include "led_manager.hpp"
#include "PowerStateTransitionHandler.hpp"
#include "rpiBootManager.hpp"
#include "relayController.hpp"
#include "SerialUartCommandSink.hpp"
#include "protocol/uartProtocol.hpp"
#include <memory>

namespace controlSystem
{
    class ActionProcessor
    {
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
        bool handleSystemCommand(const actions::ActionResponse &response);
        bool handleRelayCommand(const actions::ActionResponse &response);
        bool handleDisplayCommand(const actions::ActionResponse &response);
        bool handleBrightnessCommand(const actions::ActionResponse &response);
        
        // Component managers
        std::unique_ptr<ActionUartDispatcher> mpActionUartDispatcher;
        std::unique_ptr<SerialUartCommandSink> mpSerialUartCommandSink;
        std::unique_ptr<RpiBootManager> mpRpiBootManager;
        std::unique_ptr<RelayController> mpRelayController;
        std::unique_ptr<PowerStateTransitionHandler> mpPowerStateTransitionHandler;
        
        // References
        serialBus::Serial &mrSerial;
        relays::StandardRelay &mrRelays;
        
        static constexpr const char *mspTag = "ActionProcessor";
    };
}