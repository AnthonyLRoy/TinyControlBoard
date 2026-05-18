#include "app/ControlBoardBootstrap.hpp"

#include "board/boardConfig.hpp"
#include "indicators/ledManager.hpp"
#include <inttypes.h>

namespace controlSystem
{
    namespace
    {
        static constexpr const char *kLogTag = "Control_Board   ";
    }

    void ControlBoardBootstrap::prepareStartupIndicators() const
    {
        indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);
        // Brightness level is restored from NVS inside MonitorBrightnessController::init()
    }

    void ControlBoardBootstrap::finalizeStartupIndicators() const
    {
        ESP_LOGI(kLogTag, "ControlBoard init complete.");
        ESP_LOGI(kLogTag, "Transitioning power LED to sleep state");

        vTaskDelay(pdMS_TO_TICKS(board::timing::kInitDelayMs));

        indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);
    }

    void ControlBoardBootstrap::configureSerialCallbacks(transport::uart::UartTransport &rSerial,
                                                         SerialRxCallback onSerialRx,
                                                         VoidCallback onHeartbeatTimeout) const
    {
        rSerial.setRxCallback(std::move(onSerialRx));
        rSerial.startHeartbeatMonitor(board::timing::kHeartbeatTimeoutMs, std::move(onHeartbeatTimeout));
    }

    bool ControlBoardBootstrap::setupRelays() const
    {
        ESP_LOGI(kLogTag, "Setting up relays...");
        relays::StandardRelay::init(PIN_RELAY_SCREEN_POWER);
        relays::StandardRelay::init(PIN_RELAY_RPI_POWER);
        relays::StandardRelay::init(PIN_RELAY_DAC_POWER);
        relays::StandardRelay::init(PIN_RELAY_OUTPUT_STAGE_POWER);
        relays::StandardRelay::init(PIN_RELAY_PROTO_DAC_ENABLED);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_1);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_2);

        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_2, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_SCREEN_POWER, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_RPI_POWER, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_DAC_POWER, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_OUTPUT_STAGE_POWER, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_PROTO_DAC_ENABLED, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_1, false);

        indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);
        return true;
    }

    bool ControlBoardBootstrap::setupSerial(transport::uart::UartTransport &rSerial) const
    {
        ESP_LOGI(kLogTag, "Initializing UART");
        const bool ok = rSerial.initUart(board::serial::kPort,
                                         board::serial::kBaudRate,
                                         board::serial::kTxPin,
                                         board::serial::kRxPin,
                                         board::serial::kBufferSize,
                                         UART_PARITY_DISABLE,
                                         UART_STOP_BITS_1,
                                         UART_HW_FLOWCTRL_DISABLE);
        if (!ok)
        {
            ESP_LOGE(kLogTag, "Failed to initialize UART");
            return false;
        }

        ESP_LOGI(kLogTag, "UART initialized successfully");
        return true;
    }

    bool ControlBoardBootstrap::setupMcpHandler(buttons::McpInputHandler &rMcpHandler) const
    {
        ESP_LOGI(kLogTag, "Initializing MCP handler");
        const esp_err_t err = rMcpHandler.begin(board::i2c::kSdaPin,
                                                board::i2c::kSclPin,
                                                board::i2c::kInterruptPin);
        if (err != ESP_OK)
        {
            ESP_LOGE(kLogTag, "Failed to initialize MCP handler: %d", err);
            return false;
        }

        rMcpHandler.setTimeout(board::i2c::kMcpTimeoutMs);
        rMcpHandler.enableI2c(true);
#ifdef DEBUG_MCP_SCAN
        rMcpHandler.scanI2c();
#endif
        ESP_LOGI(kLogTag, "MCP handler initialized successfully");
#ifdef DEBUG_MCP_SCAN
        rMcpHandler.dumpRegisters();
#endif
        return true;
    }

    void ControlBoardBootstrap::configureMcpCallbacks(buttons::McpInputHandler &rMcpHandler,
                                                      ButtonCallback onButtonPressed,
                                                      ButtonCallback onButtonReleased,
                                                      RotaryCallback onRotaryMovement) const
    {
        rMcpHandler.setButtonCallback([onButtonPressed = std::move(onButtonPressed)](uint8_t pin, bool) {
            onButtonPressed(pin);
        });

        rMcpHandler.setReleaseCallback([onButtonReleased = std::move(onButtonReleased)](uint8_t pin, bool) {
            onButtonReleased(pin);
        });

        rMcpHandler.setRotaryCallback(std::move(onRotaryMovement));

        indicators::getButtonStatusLed().setStatus(ControlBoardWorkingStatus::SolidIdle);
    }
}