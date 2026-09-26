#include "app/ControlBoard.hpp"

#include "app/SerialHeartbeatRouter.hpp"
#include "indicators/ledManager.hpp"
#include "protocol/uartProtocol.hpp"
#include "hal/uart/serial.hpp"
#include "nvs_flash.h"
#include <cassert>
#include <inttypes.h>

namespace controlSystem
{

    //this in the main controlling context everything revolves around the control board
    static constexpr const char *k_logTag = "Control_Board   ";

    // Bootstraps the entire control board, wires the input pipeline, and brings the device into its active state.
    bool ControlBoard::init()
    {
        ESP_LOGI(k_logTag, "Starting ControlBoard init...");

        bootstrap::prepareStartupIndicators();
        initNvs();
        initTransport();
        initComponents();

        if (!bootstrap::setupRelays())
            return false;

        if (!bootstrap::setupMcpHandler(m_mcpHandler))
        {
            indicators::getBootDiagnosticLeds().firmwareInitFailed();
            return false;
        }

        if (!m_buttonQueue.start(*mp_inputDispatcher))
        {
            indicators::getBootDiagnosticLeds().firmwareInitFailed();
            return false;
        }

        bootstrap::configureMcpCallbacks(
            m_mcpHandler,
            [this](uint8_t pin)  { m_buttonQueue.enqueuePress(pin); },
            [this](uint8_t pin)  { m_buttonQueue.enqueueRelease(pin); },
            [this](int movement) { m_buttonQueue.enqueueRotary(movement); });

        if (!bootstrap::setupSerial(*mp_serialHandler))
        {
            indicators::getBootDiagnosticLeds().firmwareInitFailed();
            return false;
        }

        m_actionRegistry.populate(mp_buttonActions);
        bootstrap::finalizeStartupIndicators();
        if (!mp_responseProcessor->triggerInitialPowerOn())
        {
            indicators::getBootDiagnosticLeds().firmwareInitFailed();
            return false;
        }
        return true;
    }

    // Initializes the board's non-volatile settings storage, creating it if needed for persisted state.
    void ControlBoard::initNvs()
    {
        esp_err_t nvsErr = nvs_flash_init();
        if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            nvsErr = nvs_flash_init();
        }
        if (nvsErr != ESP_OK)
        {
            ESP_LOGW(k_logTag, "NVS flash init failed (0x%x) -- brightness will not persist", nvsErr);
        }
    }

    // Wires the UART and relay singletons into the control board and registers the heartbeat-monitor callbacks.
    void ControlBoard::initTransport()
    {
        mp_serialHandler = &transport::uart::UartTransport::getInstance();
        mp_relays = &relays::StandardRelay::getInstance();

        bootstrap::configureSerialCallbacks(
            *mp_serialHandler,
            [this](const UartMessage &rMsg) {
                handleSerialRxMessage(rMsg);
            },
            [this]() {
                ESP_LOGE(k_logTag, "Heartbeat timeout: No data received from Raspberry Pi within %" PRIu32 " ms",
                         board::timing::k_heartbeatTimeoutMs);
                if (mp_responseProcessor)
                {
                    mp_responseProcessor->handleHeartbeatTimeout();
                }
            });
    }

    // Creates the action processor and input dispatcher and connects them to the board callback flow.
    void ControlBoard::initComponents()
    {
        mp_responseProcessor = std::make_unique<ActionProcessor>(
            *mp_serialHandler,
            *mp_relays,
            m_systemState,
            static_cast<IControlBoardIndicators *>(this),
            [this](uint8_t buttonId) {
                if (mp_inputDispatcher)
                {
                    mp_inputDispatcher->toggleButtonLed(buttonId);
                }
            });
        mp_inputDispatcher = std::make_unique<ControlBoardInputDispatcher>(
            mp_buttonActions,
            [this](std::unique_ptr<actions::IAction> iaction) {
                process(std::move(iaction));
            },
            static_cast<IControlBoardIndicators &>(*this));
    }

    // Tears down the control board tasks and hardware interfaces before a shutdown or reset.
    void ControlBoard::deinit()
    {
        m_buttonQueue.stop();
        if (mp_serialHandler != nullptr)
        {
            mp_serialHandler->deinitUart();
            mp_serialHandler = nullptr;
        }
        mp_inputDispatcher.reset();
        mp_responseProcessor.reset();
    }

    // Delegates a board action into the main action-processing pipeline for execution.
    void ControlBoard::process(std::unique_ptr<actions::IAction> iaction)
    {
        assert(mp_responseProcessor != nullptr);
        mp_responseProcessor->process(std::move(iaction));
    }

    // Updates the global board activity status and resets toggle state when the system sleeps.
    void ControlBoard::setActivityStatus(ControlBoardWorkingStatus status)
    {
        if (status != ControlBoardWorkingStatus::doingWork)
        {
            assert(mp_inputDispatcher != nullptr);
            mp_inputDispatcher->setBackgroundStatus(status);
            if (status == ControlBoardWorkingStatus::sleeping)
            {
                m_actionRegistry.resetActionStates();
                if (mp_responseProcessor)
                {
                    mp_responseProcessor->resetToggleStates();
                }
            }
        }
        indicators::getActivityStatusLed().sendStatus(status);
    }

    // Sets the physical LED state for a specific button as part of board feedback and system-state tracking.
    void ControlBoard::setButtonLed(uint8_t pin, bool enabled)
    {
        indicators::getSpiLedDriver().setLed(pin, enabled);
        const auto mask = static_cast<uint16_t>(1u << pin);
        if (enabled)
            m_systemState.buttonLedBitmask.fetch_or(mask, std::memory_order_relaxed);
        else
            m_systemState.buttonLedBitmask.fetch_and(static_cast<uint16_t>(~mask), std::memory_order_relaxed);
    }

    // Reports a healthy heartbeat to the power/boot logic so the board keeps its active state.
    void ControlBoard::handleHeartbeatReceived()
    {
        assert(mp_responseProcessor != nullptr);
        mp_responseProcessor->handleHeartbeatReceived();
    }

    // Decodes incoming UART traffic, updates runtime status, and routes protocol events to the right handler.
    void ControlBoard::handleSerialRxMessage(const UartMessage &rMsg)
    {
        ESP_LOGI(k_logTag, "Received UART message: cmd=0x%04X seq=%u type=%u",
                 rMsg.commandId, rMsg.sequence, rMsg.msgType);

        if (rMsg.msgType == MSG_NOW_PLAYING)
        {
            memcpy(m_systemState.nowPlayingText, rMsg.nowPlayingText, rMsg.nowPlayingLen + 1);
            m_systemState.nowPlayingVersion.fetch_add(1, std::memory_order_release);
            ESP_LOGI(k_logTag, "Now playing: %s", m_systemState.nowPlayingText);
            return;
        }

        if (rMsg.msgType == MSG_TRACK_PROGRESS)
        {
            m_systemState.trackProgressUpdating.store(true, std::memory_order_seq_cst);
            m_systemState.trackElapsedSeconds.store(rMsg.trackElapsedSec, std::memory_order_relaxed);
            m_systemState.trackDurationSeconds.store(rMsg.trackDurationSec, std::memory_order_relaxed);
            m_systemState.trackIsPlaying.store(rMsg.trackIsPlaying, std::memory_order_relaxed);
            m_systemState.trackProgressVersion.fetch_add(1, std::memory_order_release);
            m_systemState.trackProgressUpdating.store(false, std::memory_order_seq_cst);
            return;
        }

        if (rMsg.msgType == MSG_LIBRARY_ENTRY)
        {
            if (m_libraryEntryCallback)
                m_libraryEntryCallback(rMsg);
            return;
        }

        if (rMsg.msgType == MSG_PLAYLIST_RESULT)
        {
            if (m_playlistResultCallback)
                m_playlistResultCallback(rMsg);
            return;
        }

        if (isHeartbeatCommand(rMsg.commandId))
        {
            handleHeartbeatReceived();
            if (rMsg.commandId == kLegacyHeartbeatCommandId)
            {
                ESP_LOGW(k_logTag, "Received legacy heartbeat command 0x%04X; update the RPi heartbeat sender to CMD_SYS_HEARTBEAT (0x%04X)",
                         rMsg.commandId, CMD_SYS_HEARTBEAT);
            }
            return;
        }

        assert(mp_responseProcessor != nullptr);
        if (!mp_responseProcessor->handleInboundUartMessage(rMsg))
        {
            ESP_LOGI(k_logTag, "No inbound handler implemented for UART message (cmd=0x%04X, type=%u)",
                     rMsg.commandId, rMsg.msgType);
        }
    }
}
