#include "app/ControlBoard.hpp"

#include "indicators/ledManager.hpp"
#include "protocol/uartProtocol.hpp"
#include "transport/uart/serial.hpp"
#include "nvs_flash.h"
#include <cassert>
#include <inttypes.h>

namespace controlSystem
{
    static constexpr const char *k_logTag = "Control_Board   ";

    bool ControlBoard::init()
    {
        ESP_LOGI(k_logTag, "Starting ControlBoard init...");

        initNvs();
        m_bootstrap.prepareStartupIndicators();
        initTransport();
        initComponents();

        if (!m_bootstrap.setupRelays())
            return false;

        if (!m_bootstrap.setupMcpHandler(m_mcpHandler))
        {
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        if (!m_buttonQueue.start(*mp_inputDispatcher))
        {
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        m_bootstrap.configureMcpCallbacks(
            m_mcpHandler,
            [this](uint8_t pin)  { m_buttonQueue.enqueuePress(pin); },
            [this](uint8_t pin)  { m_buttonQueue.enqueueRelease(pin); },
            [this](int movement) { m_buttonQueue.enqueueRotary(movement); });

        if (!m_bootstrap.setupSerial(*mp_serialHandler))
        {
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        m_actionRegistry.populate(mp_buttonActions);
        m_bootstrap.finalizeStartupIndicators();
        mp_responseProcessor->triggerInitialPowerOn();
        return true;
    }

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
            ESP_LOGW(k_logTag, "NVS flash init failed (0x%x) u{2014} brightness will not persist", nvsErr);
        }
    }

    void ControlBoard::initTransport()
    {
        mp_serialHandler = &transport::uart::UartTransport::getInstance();
        mp_relays = &relays::StandardRelay::getInstance();

        m_bootstrap.configureSerialCallbacks(
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

    void ControlBoard::initComponents()
    {
        mp_responseProcessor = std::make_unique<ActionProcessor>(
            *mp_serialHandler,
            *mp_relays,
            m_systemState,
            static_cast<IControlBoardIndicators *>(this));
        mp_inputDispatcher = std::make_unique<ControlBoardInputDispatcher>(
            mp_buttonActions,
            static_cast<IActionResponseSink &>(*this),
            static_cast<IControlBoardIndicators &>(*this),
            m_systemState);
        mp_heartbeatRouter = std::make_unique<SerialHeartbeatRouter>(static_cast<IHeartbeatSink *>(this));
    }

    void ControlBoard::deinit()
    {
        m_buttonQueue.stop();
        assert(mp_serialHandler != nullptr);
        mp_serialHandler->deinitUart();
        mp_serialHandler = nullptr;
        mp_heartbeatRouter.reset();
        mp_inputDispatcher.reset();
        mp_responseProcessor.reset();
    }

    void ControlBoard::process(const actions::ActionResponse &response)
    {
        assert(mp_responseProcessor != nullptr);
        mp_responseProcessor->process(response);
    }

    void ControlBoard::setActivityStatus(ControlBoardWorkingStatus status)
    {
        if (status != ControlBoardWorkingStatus::doingWork)
        {
            assert(mp_inputDispatcher != nullptr);
            mp_inputDispatcher->setBackgroundStatus(status);
        }
        indicators::getActivityStatusLed().sendStatus(status);
    }

    void ControlBoard::setButtonLed(uint8_t pin, bool enabled)
    {
        indicators::getSpiLedDriver().setLed(pin, enabled);
    }

    void ControlBoard::handleHeartbeatReceived()
    {
        assert(mp_responseProcessor != nullptr);
        mp_responseProcessor->handleHeartbeatReceived();
    }

    void ControlBoard::handleSerialRxMessage(const UartMessage &rMsg)
    {
        ESP_LOGI(k_logTag, "Received UART message: cmd=0x%04X seq=%u type=%u",
                 rMsg.commandId, rMsg.sequence, rMsg.msgType);

        assert(mp_heartbeatRouter != nullptr);
        if (mp_heartbeatRouter->route(rMsg))
        {
            if (rMsg.commandId == SerialHeartbeatRouter::k_legacyHeartbeatCommandId)
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
