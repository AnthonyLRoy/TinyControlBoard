#include "app/ControlBoard.hpp"

#include "indicators/ledManager.hpp"
#include "protocol/uartProtocol.hpp"
#include "transport/uart/serial.hpp"
#include "nvs_flash.h"
#include <inttypes.h>

namespace controlSystem
{
    namespace
    {
        constexpr uint32_t k_actionTaskStackSize = 4096;
        constexpr UBaseType_t k_actionTaskPriority = 5;
    }

    static constexpr const char *k_logTag = "Control_Board   ";

    bool ControlBoard::init()
    {
        ESP_LOGI(k_logTag, "Starting ControlBoard init...");

        // Initialise NVS flash (required before any nvs_open call)
        esp_err_t nvsErr = nvs_flash_init();
        if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            nvsErr = nvs_flash_init();
        }
        if (nvsErr != ESP_OK)
        {
            ESP_LOGW(k_logTag, "NVS flash init failed (0x%x) — brightness will not persist", nvsErr);
        }

        m_bootstrap.prepareStartupIndicators();

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

        mp_responseProcessor = std::make_unique<ActionProcessor>(
            *mp_serialHandler,
            *mp_relays,
            m_systemState,
            static_cast<IActivityStatusSink *>(static_cast<IControlBoardIndicators *>(this)));
        mp_inputDispatcher = std::make_unique<ControlBoardInputDispatcher>(
            mp_buttonActions,
            static_cast<IActionResponseSink &>(*this),
            static_cast<IControlBoardIndicators &>(*this),
            m_systemState);
        mp_heartbeatRouter = std::make_unique<SerialHeartbeatRouter>(static_cast<IHeartbeatSink *>(this));

        if (!m_bootstrap.setupRelays())
        {
            return false;
        }

        if (!m_bootstrap.setupMcpHandler(m_mcpHandler))
        {
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        m_buttonEventQueue = xQueueCreate(k_buttonQueueDepth, sizeof(ButtonEvent));
        if (!m_buttonEventQueue)
        {
            ESP_LOGE(k_logTag, "Failed to create button event queue");
            return false;
        }
        if (xTaskCreate(actionTask, "action_task", k_actionTaskStackSize, this, k_actionTaskPriority, &m_actionTaskHandle) != pdPASS)
        {
            ESP_LOGE(k_logTag, "Failed to create action task");
            vQueueDelete(m_buttonEventQueue);
            m_buttonEventQueue = nullptr;
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        m_bootstrap.configureMcpCallbacks(
            m_mcpHandler,
            [this](uint8_t pin) {
                const ButtonEvent event{ButtonEventType::Press, pin, 0};
                enqueueButtonEvent(event, "press");
            },
            [this](uint8_t pin) {
                const ButtonEvent event{ButtonEventType::Release, pin, 0};
                enqueueButtonEvent(event, "release");
            },
            [this](int movement) {
                const ButtonEvent event{ButtonEventType::Rotary, 0, static_cast<int8_t>(movement)};
                enqueueButtonEvent(event, "rotary");
            });

        if (!m_bootstrap.setupSerial(*mp_serialHandler))
        {
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        m_actionRegistry.populate(mp_buttonActions);

        m_bootstrap.finalizeStartupIndicators();

        return true;
    }

    void ControlBoard::deinit()
    {
        if (m_actionTaskHandle)
        {
            vTaskDelete(m_actionTaskHandle);
            m_actionTaskHandle = nullptr;
        }
        if (m_buttonEventQueue)
        {
            vQueueDelete(m_buttonEventQueue);
            m_buttonEventQueue = nullptr;
        }
        if (mp_serialHandler)
        {
            mp_serialHandler->deinitUart();
            mp_serialHandler = nullptr;
        }
        mp_heartbeatRouter.reset();
        mp_inputDispatcher.reset();
        mp_responseProcessor.reset();
    }

    bool ControlBoard::enqueueButtonEvent(const ButtonEvent &event, const char *p_eventName)
    {
        if (!m_buttonEventQueue)
        {
            ESP_LOGW(k_logTag, "Dropping %s event because queue is not initialized", p_eventName);
            return false;
        }

        if (xQueueSend(m_buttonEventQueue, &event, 0) == pdTRUE)
        {
            return true;
        }

        ++m_droppedButtonEvents;
        if ((m_droppedButtonEvents % 16U) == 1U)
        {
            ESP_LOGW(k_logTag, "Button event queue full, dropped %lu events (latest=%s)",
                     static_cast<unsigned long>(m_droppedButtonEvents), p_eventName);
        }
        return false;
    }

    void ControlBoard::actionTask(void *pvParam)
    {
        auto *p_self = static_cast<ControlBoard *>(pvParam);
        ButtonEvent event{};
        while (true)
        {
            if (xQueueReceive(p_self->m_buttonEventQueue, &event, portMAX_DELAY) == pdTRUE)
            {
                if (!p_self->mp_inputDispatcher)
                {
                    continue;
                }
                switch (event.type)
                {
                case ButtonEventType::Press:
                    p_self->mp_inputDispatcher->handleButtonPressed(event.buttonId);
                    break;
                case ButtonEventType::Release:
                    p_self->mp_inputDispatcher->handleButtonReleased(event.buttonId);
                    break;
                case ButtonEventType::Rotary:
                    p_self->mp_inputDispatcher->handleRotaryMovement(event.rotaryDelta);
                    break;
                }
            }
        }
    }

    void ControlBoard::process(const actions::ActionResponse &response)
    {
        if (mp_responseProcessor)
        {
            mp_responseProcessor->process(response);
        }
    }

    void ControlBoard::setActivityStatus(ControlBoardWorkingStatus status)
    {
        if (status != ControlBoardWorkingStatus::doingWork)
        {
            m_backgroundStatus = status;
            if (mp_inputDispatcher)
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
        if (mp_responseProcessor)
        {
            mp_responseProcessor->handleHeartbeatReceived();
        }
    }

    void ControlBoard::handleSerialRxMessage(const UartMessage &rMsg)
    {
        ESP_LOGI(k_logTag, "Received UART message: cmd=0x%04X seq=%u type=%u",
                 rMsg.commandId, rMsg.sequence, rMsg.msgType);

        if (mp_heartbeatRouter && mp_heartbeatRouter->route(rMsg))
        {
            if (rMsg.commandId == SerialHeartbeatRouter::k_legacyHeartbeatCommandId)
            {
                ESP_LOGW(k_logTag, "Received legacy heartbeat command 0x%04X; update the RPi heartbeat sender to CMD_SYS_HEARTBEAT (0x%04X)",
                         rMsg.commandId, CMD_SYS_HEARTBEAT);
            }
            return;
        }

        if (mp_responseProcessor)
        {
            if (!mp_responseProcessor->handleInboundUartMessage(rMsg))
            {
                ESP_LOGI(k_logTag, "No inbound handler implemented for UART message (cmd=0x%04X, type=%u)",
                         rMsg.commandId, rMsg.msgType);
            }
        }
    }
}