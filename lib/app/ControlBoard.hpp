#pragma once

#include "indicators/powerLed.hpp"
#include "indicators/statusLed.hpp"
#include "hal/relay/relay.hpp"
#include "input/input.hpp"
#include "hal/uart/serial.hpp"
#include "app/actionProcessor.hpp"
#include "app/ControlBoardActionRegistry.hpp"
#include "app/ControlBoardBootstrap.hpp"
#include "app/ControlBoardInputDispatcher.hpp"
#include "app/ButtonEventQueue.hpp"
#include "app/SystemState.hpp"
#include "board/boardConfig.hpp"
#include "freertos/FreeRTOS.h"
#include <array>
#include <functional>
#include <memory>

namespace controlSystem
{
    class ControlBoard : private IControlBoardIndicators
    {
    public:
        // Initializes the full control board stack and its connected subsystems.
        bool init();
        // Tears down board resources and stops active tasks before shutdown.
        void deinit();
        // Returns the active action processor used to handle command dispatch.
        ActionProcessor &getActionProcessor() { return *mp_responseProcessor; }
        // Returns the live shared runtime state for board subscribers.
        SystemState &getSystemState() { return m_systemState; }
        // Registers the callback invoked when a library-entry UART message arrives.
        void setLibraryEntryCallback(std::function<void(const UartMessage &)> callback)
        {
            m_libraryEntryCallback = std::move(callback);
        }
        // Registers the callback invoked when a playlist-result UART message arrives.
        void setPlaylistResultCallback(std::function<void(const UartMessage &)> callback)
        {
            m_playlistResultCallback = std::move(callback);
        }

    private:
        // Initializes the non-volatile storage used for persisted board settings.
        void initNvs();
        // Initializes the UART transport and attached board communication hardware.
        void initTransport();
        // Creates and wires the board components that process input and output.
        void initComponents();

        // Executes a queued action through the control-board processing pipeline.
        void process(std::unique_ptr<actions::IAction> iaction);
        // Applies a new board activity status to the indicators sink.
        void setActivityStatus(ControlBoardWorkingStatus status) override;
        // Updates the LED state for the specified button pin.
        void setButtonLed(uint8_t pin, bool enabled) override;
        // Handles a heartbeat event indicating the Raspberry Pi is still healthy.
        void handleHeartbeatReceived();
        // Decodes and dispatches an incoming serial message to the appropriate handler.
        void handleSerialRxMessage(const UartMessage &rMsg);

        transport::uart::UartTransport *mp_serialHandler = nullptr;  // non-owning; singleton assigned in initTransport()
        relays::StandardRelay *mp_relays = nullptr;                   // non-owning; singleton assigned in initTransport()
        std::unique_ptr<ActionProcessor> mp_responseProcessor;
        std::unique_ptr<ControlBoardInputDispatcher> mp_inputDispatcher;
        ControlBoardActionRegistry m_actionRegistry;

        buttons::McpInputHandler m_mcpHandler{board::i2c::k_mcpAddress, I2C_NUM_0};
        ControlBoardInputDispatcher::ActionMap mp_buttonActions{};
        ButtonEventQueue m_buttonQueue;

        SystemState m_systemState;
        std::function<void(const UartMessage &)> m_libraryEntryCallback;
        std::function<void(const UartMessage &)> m_playlistResultCallback;
    };
}