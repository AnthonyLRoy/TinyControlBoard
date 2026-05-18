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
        constexpr uint32_t kActionTaskStackSize = 4096;
        constexpr UBaseType_t kActionTaskPriority = 5;
    }

    static constexpr const char *kLogTag = "Control_Board   ";

    bool ControlBoard::init()
    {
        ESP_LOGI(kLogTag, "Starting ControlBoard init...");

        // Initialise NVS flash (required before any nvs_open call)
        esp_err_t nvsErr = nvs_flash_init();
        if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            nvsErr = nvs_flash_init();
        }
        if (nvsErr != ESP_OK)
        {
            ESP_LOGW(kLogTag, "NVS flash init failed (0x%x) — brightness will not persist", nvsErr);
        }

        mBootstrap.prepareStartupIndicators();

        mpSerialHandler = &transport::uart::UartTransport::getInstance();
        mpRelays = &relays::StandardRelay::getInstance();

        mBootstrap.configureSerialCallbacks(
            *mpSerialHandler,
            [this](const UartMessage &rMsg) {
                handleSerialRxMessage(rMsg);
            },
            [this]() {
                ESP_LOGE(kLogTag, "Heartbeat timeout: No data received from Raspberry Pi within %" PRIu32 " ms",
                         board::timing::kHeartbeatTimeoutMs);
                if (mpResponseProcessor)
                {
                    mpResponseProcessor->handleHeartbeatTimeout();
                }
            });

        mpResponseProcessor = std::make_unique<ActionProcessor>(
            *mpSerialHandler,
            *mpRelays,
            mSystemState,
            static_cast<IActivityStatusSink *>(static_cast<IControlBoardIndicators *>(this)));
        mpInputDispatcher = std::make_unique<ControlBoardInputDispatcher>(
            mpButtonActions,
            static_cast<IActionResponseSink &>(*this),
            static_cast<IControlBoardIndicators &>(*this),
            mSystemState);
        mpHeartbeatRouter = std::make_unique<SerialHeartbeatRouter>(static_cast<IHeartbeatSink *>(this));

        if (!mBootstrap.setupRelays())
        {
            return false;
        }

        if (!mBootstrap.setupMcpHandler(mMcpHandler))
        {
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        mButtonEventQueue = xQueueCreate(kButtonQueueDepth, sizeof(ButtonEvent));
        if (!mButtonEventQueue)
        {
            ESP_LOGE(kLogTag, "Failed to create button event queue");
            return false;
        }
        if (xTaskCreate(actionTask, "action_task", kActionTaskStackSize, this, kActionTaskPriority, &mActionTaskHandle) != pdPASS)
        {
            ESP_LOGE(kLogTag, "Failed to create action task");
            vQueueDelete(mButtonEventQueue);
            mButtonEventQueue = nullptr;
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        mBootstrap.configureMcpCallbacks(
            mMcpHandler,
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

        if (!mBootstrap.setupSerial(*mpSerialHandler))
        {
            indicators::getSpiBootIndicator().notifyFailure();
            return false;
        }

        mActionRegistry.populate(mpButtonActions);

        mBootstrap.finalizeStartupIndicators();

        return true;
    }

    void ControlBoard::deinit()
    {
        if (mActionTaskHandle)
        {
            vTaskDelete(mActionTaskHandle);
            mActionTaskHandle = nullptr;
        }
        if (mButtonEventQueue)
        {
            vQueueDelete(mButtonEventQueue);
            mButtonEventQueue = nullptr;
        }
        if (mpSerialHandler)
        {
            mpSerialHandler->deinitUart();
            mpSerialHandler = nullptr;
        }
        mpHeartbeatRouter.reset();
        mpInputDispatcher.reset();
        mpResponseProcessor.reset();
    }

    bool ControlBoard::enqueueButtonEvent(const ButtonEvent &event, const char *pEventName)
    {
        if (!mButtonEventQueue)
        {
            ESP_LOGW(kLogTag, "Dropping %s event because queue is not initialized", pEventName);
            return false;
        }

        if (xQueueSend(mButtonEventQueue, &event, 0) == pdTRUE)
        {
            return true;
        }

        ++mDroppedButtonEvents;
        if ((mDroppedButtonEvents % 16U) == 1U)
        {
            ESP_LOGW(kLogTag, "Button event queue full, dropped %lu events (latest=%s)",
                     static_cast<unsigned long>(mDroppedButtonEvents), pEventName);
        }
        return false;
    }

    void ControlBoard::actionTask(void *pvParam)
    {
        auto *pSelf = static_cast<ControlBoard *>(pvParam);
        ButtonEvent event{};
        while (true)
        {
            if (xQueueReceive(pSelf->mButtonEventQueue, &event, portMAX_DELAY) == pdTRUE)
            {
                if (!pSelf->mpInputDispatcher)
                {
                    continue;
                }
                switch (event.type)
                {
                case ButtonEventType::Press:
                    pSelf->mpInputDispatcher->handleButtonPressed(event.buttonId);
                    break;
                case ButtonEventType::Release:
                    pSelf->mpInputDispatcher->handleButtonReleased(event.buttonId);
                    break;
                case ButtonEventType::Rotary:
                    pSelf->mpInputDispatcher->handleRotaryMovement(event.rotaryDelta);
                    break;
                }
            }
        }
    }

    void ControlBoard::process(const actions::ActionResponse &response)
    {
        if (mpResponseProcessor)
        {
            mpResponseProcessor->process(response);
        }
    }

    void ControlBoard::setActivityStatus(ControlBoardWorkingStatus status)
    {
        if (status != ControlBoardWorkingStatus::doingWork)
        {
            mBackgroundStatus = status;
            if (mpInputDispatcher)
                mpInputDispatcher->setBackgroundStatus(status);
        }
        indicators::getActivityStatusLed().sendStatus(status);
    }

    void ControlBoard::setButtonLed(uint8_t pin, bool enabled)
    {
        indicators::getSpiLedDriver().setLed(pin, enabled);
    }

    void ControlBoard::handleHeartbeatReceived()
    {
        if (mpResponseProcessor)
        {
            mpResponseProcessor->handleHeartbeatReceived();
        }
    }

    void ControlBoard::handleSerialRxMessage(const UartMessage &rMsg)
    {
        ESP_LOGI(kLogTag, "Received UART message: cmd=0x%04X seq=%u type=%u",
                 rMsg.commandId, rMsg.sequence, rMsg.msgType);

        if (mpHeartbeatRouter && mpHeartbeatRouter->route(rMsg))
        {
            if (rMsg.commandId == SerialHeartbeatRouter::kLegacyHeartbeatCommandId)
            {
                ESP_LOGW(kLogTag, "Received legacy heartbeat command 0x%04X; update the RPi heartbeat sender to CMD_SYS_HEARTBEAT (0x%04X)",
                         rMsg.commandId, CMD_SYS_HEARTBEAT);
            }
            return;
        }

        if (mpResponseProcessor)
        {
            if (!mpResponseProcessor->handleInboundUartMessage(rMsg))
            {
                ESP_LOGI(kLogTag, "No inbound handler implemented for UART message (cmd=0x%04X, type=%u)",
                         rMsg.commandId, rMsg.msgType);
            }
        }
    }
}