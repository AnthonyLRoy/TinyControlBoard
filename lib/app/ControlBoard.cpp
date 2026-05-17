#include "app/ControlBoard.hpp"

#include "indicators/ledManager.hpp"
#include "protocol/uartProtocol.hpp"
#include "transport/uart/serial.hpp"
#include "nvs_flash.h"
#include <inttypes.h>

namespace controlSystem
{
    static const char *spTag = "Control_Board   ";

    bool ControlBoard::init()
    {
        ESP_LOGI(spTag, "Starting ControlBoard init...");

        // Initialise NVS flash (required before any nvs_open call)
        esp_err_t nvsErr = nvs_flash_init();
        if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            nvs_flash_erase();
            nvsErr = nvs_flash_init();
        }
        if (nvsErr != ESP_OK)
        {
            ESP_LOGW(spTag, "NVS flash init failed (0x%x) — brightness will not persist", nvsErr);
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
                ESP_LOGE(spTag, "Heartbeat timeout: No data received from Raspberry Pi within %" PRIu32 " ms",
                         board::timing::kHeartbeatTimeoutMs);
                if (mpResponseProcessor)
                {
                    mpResponseProcessor->handleHeartbeatTimeout();
                }
            });

        mpResponseProcessor = std::make_unique<ActionProcessor>(*mpSerialHandler, *mpRelays);
        mpInputDispatcher = std::make_unique<ControlBoardInputDispatcher>(
            mpButtonActions,
            static_cast<IActionResponseSink *>(this),
            static_cast<IControlBoardIndicators *>(this));
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

        mBootstrap.configureMcpCallbacks(
            mMcpHandler,
            [this](uint8_t pin) {
                if (mpInputDispatcher)
                {
                    mpInputDispatcher->handleButtonPressed(pin);
                }
            },
            [this](uint8_t pin) {
                if (mpInputDispatcher)
                {
                    mpInputDispatcher->handleButtonReleased(pin);
                }
            },
            [this](int movement) {
                if (mpInputDispatcher)
                {
                    mpInputDispatcher->handleRotaryMovement(movement);
                }
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
        if (mpSerialHandler)
        {
            mpSerialHandler->deinitUart();
            mpSerialHandler = nullptr;
        }
        mpHeartbeatRouter.reset();
        mpInputDispatcher.reset();
        mpResponseProcessor.reset();
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
        ESP_LOGI(spTag, "Received UART message - Command ID: 0x%04X, Sequence: %u, Type: %u",
                 rMsg.commandId, rMsg.sequence, rMsg.msgType);

        if (mpHeartbeatRouter && mpHeartbeatRouter->route(rMsg))
        {
            if (rMsg.commandId == SerialHeartbeatRouter::kLegacyHeartbeatCommandId)
            {
                ESP_LOGW(spTag, "Received legacy heartbeat command 0x%04X; update the RPI heartbeat sender to CMD_SYS_HEARTBEAT (0x%04X)",
                         rMsg.commandId, CMD_SYS_HEARTBEAT);
            }
            return;
        }

        if (mpResponseProcessor)
        {
            ESP_LOGI(spTag, "Processing UART message through action processor (cmd=0x%04X)", rMsg.commandId);
        }
    }
}