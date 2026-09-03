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
        bool init();
        void deinit();
        ActionProcessor &getActionProcessor() { return *mp_responseProcessor; }
        SystemState &getSystemState() { return m_systemState; }
        void setLibraryEntryCallback(std::function<void(const UartMessage &)> callback)
        {
            m_libraryEntryCallback = std::move(callback);
        }
        void setPlaylistResultCallback(std::function<void(const UartMessage &)> callback)
        {
            m_playlistResultCallback = std::move(callback);
        }

    private:
        void initNvs();
        void initTransport();
        void initComponents();

        void process(std::unique_ptr<actions::IAction> iaction);
        void setActivityStatus(ControlBoardWorkingStatus status) override;
        void setButtonLed(uint8_t pin, bool enabled) override;
        void handleHeartbeatReceived();
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