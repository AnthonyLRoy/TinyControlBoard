#include "app/ControlBoardBootstrap.hpp"

#include "board/boardConfig.hpp"
#include "indicators/ledManager.hpp"
#include <inttypes.h>

namespace controlSystem
{
    namespace
    {
        static constexpr const char *k_logTag = "Control_Board   ";
    }

    namespace bootstrap
    {
        void prepareStartupIndicators()
        {
            indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);
            indicators::getSpiLedDriver().setAllLeds(false);
            // Flash all button LEDs on for 1 second at the very start of bootup.
            indicators::getSpiLedDriver().setAllLeds(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            indicators::getSpiLedDriver().setAllLeds(false);
            // Brightness level is restored from NVS inside MonitorBrightnessController::init()
            indicators::getBootDiagnosticLeds().begin();
        }

        void finalizeStartupIndicators()
        {
            ESP_LOGI(k_logTag, "ControlBoard init complete.");
            vTaskDelay(pdMS_TO_TICKS(board::timing::k_initDelayMs));
        }

        void configureSerialCallbacks(transport::uart::UartTransport &rSerial,
                                      SerialRxCallback onSerialRx,
                                      VoidCallback onHeartbeatTimeout)
        {
            rSerial.setRxCallback(std::move(onSerialRx));
            rSerial.startHeartbeatMonitor(board::timing::k_heartbeatTimeoutMs, std::move(onHeartbeatTimeout));
        }

        bool setupRelays()
        {
            ESP_LOGI(k_logTag, "Setting up relays...");
            relays::StandardRelay::init(PIN_RELAY_VCC_3V3_POWER);
            relays::StandardRelay::init(PIN_RELAY_RPI_POWER);
            relays::StandardRelay::init(PIN_RELAY_DAC_POWER);
            relays::StandardRelay::init(PIN_RELAY_OUTPUT_STAGE_POWER);
            relays::StandardRelay::init(PIN_RELAY_ESS_DAC_ENABLED);
            relays::StandardRelay::init(PIN_RELAY_GENERAL_1);
            relays::StandardRelay::init(PIN_RELAY_GENERAL_2);

            relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_2, false);
            relays::StandardRelay::setRelayState(PIN_RELAY_VCC_3V3_POWER, false);
            relays::StandardRelay::setRelayState(PIN_RELAY_RPI_POWER, false);
            relays::StandardRelay::setRelayState(PIN_RELAY_DAC_POWER, false);
            relays::StandardRelay::setRelayState(PIN_RELAY_OUTPUT_STAGE_POWER, false);
            relays::StandardRelay::setRelayState(PIN_RELAY_ESS_DAC_ENABLED, false);
            relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_1, false);

            indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);
            return true;
        }

        bool setupSerial(transport::uart::UartTransport &rSerial)
        {
            ESP_LOGI(k_logTag, "Initializing UART");
            const bool ok = rSerial.initUart(board::serial::k_port,
                                             board::serial::k_baudRate,
                                             board::serial::k_txPin,
                                             board::serial::k_rxPin,
                                             board::serial::k_bufferSize,
                                             UART_PARITY_DISABLE,
                                             UART_STOP_BITS_1,
                                             UART_HW_FLOWCTRL_DISABLE);
            if (!ok)
            {
                ESP_LOGE(k_logTag, "Failed to initialize UART");
                return false;
            }

            ESP_LOGI(k_logTag, "UART initialized successfully");
            return true;
        }

        bool setupMcpHandler(buttons::McpInputHandler &rMcpHandler)
        {
            ESP_LOGI(k_logTag, "Initializing MCP handler");
            const esp_err_t err = rMcpHandler.begin(board::i2c::k_sdaPin,
                                                    board::i2c::k_sclPin,
                                                    board::i2c::k_interruptPin);
            if (err != ESP_OK)
            {
                ESP_LOGE(k_logTag, "Failed to initialize MCP handler: %d", err);
                return false;
            }

            rMcpHandler.setTimeout(board::i2c::k_mcpTimeoutMs);
            rMcpHandler.enableI2c(true);
#ifdef DEBUG_MCP_SCAN
            rMcpHandler.scanI2c();
#endif
            ESP_LOGI(k_logTag, "MCP handler initialized successfully");
#ifdef DEBUG_MCP_SCAN
            rMcpHandler.dumpRegisters();
#endif
            return true;
        }

        void configureMcpCallbacks(buttons::McpInputHandler &rMcpHandler,
                                   ButtonCallback onButtonPressed,
                                   ButtonCallback onButtonReleased,
                                   RotaryCallback onRotaryMovement)
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
}