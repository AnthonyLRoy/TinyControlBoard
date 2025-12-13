#include "ControlBoard.hpp"
#include "led_Manager.hpp"
#include "uart_protocol.hpp"
#include "actionProcessor.hpp"
#include "serial.hpp"
#include "spi.hpp"
#include <inttypes.h>

namespace controlSystem
{
    static const char *TAG = "CONTROL_BOARD";

    bool ControlBoard::init()
    {
        ESP_LOGI(TAG, "Starting ControlBoard init...");

        // Set initial power state
        indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);

        // Initialize serial handler and heartbeat monitor
        serialHandler = &serialBus::Serial::instance();
        
        // Register UART RX callback
        serialHandler->set_rx_callback([this](const UARTMessage &msg) {
            this->handleSerialRxMessage(msg);
        });

        serialHandler->start_heartbeat_monitor(ControlBoardConfig::HEARTBEAT_TIMEOUT_MS, [this]() {
            ESP_LOGE(TAG, "Heartbeat timeout: No data received from Raspberry Pi within %" PRIu32 " ms",
                     ControlBoardConfig::HEARTBEAT_TIMEOUT_MS);
            // Notify action processor that heartbeat timeout occurred (RPI is offline)
            if (responseProcessor) {
                responseProcessor->onHeartbeatTimeout();
            }
        });

        // Initialize SPI and action processor

        responseProcessor = new actionProcessor(*serialHandler, *relays);

        // Setup hardware components with error checking
        if (!setupRelays()) {
            ESP_LOGE(TAG, "Failed to setup relays");
            return false;
        }

        if (!setupMCPHandler()) {
            ESP_LOGE(TAG, "Failed to setup MCP handler");
            return false;
        }

        setupMCPCallbacks();

        if (!setupSerial()) {
            ESP_LOGE(TAG, "Failed to setup serial");
            return false;
        }

        createButtonActionMap();

        ESP_LOGI(TAG, "ControlBoard init complete.");
        ESP_LOGI(TAG, "Transitioning Power LED to Sleep state...");
        vTaskDelay(pdMS_TO_TICKS(ControlBoardConfig::INIT_DELAY_MS));
        indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);

        return true;
    }

    void ControlBoard::deinit()
    {
        if (serialHandler)
        {
            serialHandler->deinit_uart();
            serialHandler = nullptr;
        }
        delete responseProcessor;
        responseProcessor = nullptr;
    }

    bool ControlBoard::setupRelays()
    {
        ESP_LOGI(TAG, "Setting up relays...");
        relays::StandardRelay::init(PIN_RELAY_SCREEN);
        relays::StandardRelay::init(PIN_RELAY_RPI);
        relays::StandardRelay::init(PIN_RELAY_DAC);
        relays::StandardRelay::init(PIN_RELAY_OUTPUT_STAGE);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_3);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_4);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_1);

        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_1, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_SCREEN, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_RPI, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_DAC, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_OUTPUT_STAGE, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_3, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_4, false);

        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::Idle);

        return true;
    }

    bool ControlBoard::setupSerial()
    {
        ESP_LOGI(TAG, "Initializing serial...");
        bool ok = serialHandler->init_uart(ControlBoardConfig::UART_NUM,
                                           ControlBoardConfig::UART_BOARD_RATE,
                                           ControlBoardConfig::PIN_SERIAL_TX,
                                           ControlBoardConfig::PIN_SERIAL_RX,
                                           256,
                                           UART_PARITY_DISABLE,
                                           UART_STOP_BITS_1,
                                           UART_HW_FLOWCTRL_DISABLE);
        if (!ok)
        {
            ESP_LOGE(TAG, "Failed to initialize UART");
            return false;
        }
        ESP_LOGI(TAG, "UART initialized successfully");
        return true;
    }


    bool ControlBoard::setupMCPHandler()
    {
        ESP_LOGI(TAG, "Initializing MCP handler...");
        esp_err_t err = mcpHandler.begin(ControlBoardConfig::PIN_I2C_SDA,
                                         ControlBoardConfig::PIN_I2C_SCL,
                                         ControlBoardConfig::PIN_I2C_INT);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed MCPHandler begin: %d", err);
            return false;
        }
        mcpHandler.setTimeout(ControlBoardConfig::MCP_TIMEOUT_MS);
        mcpHandler.I2CEnable(true);
#ifdef DEBUG_MCP_SCAN
        mcpHandler.scanner();
#endif
        ESP_LOGI(TAG, "MCP Handler initialized successfully.");
#ifdef DEBUG_MCP_SCAN
        mcpHandler.dumpRegisters();
#endif
        return true;
    }

    void ControlBoard::setupMCPCallbacks()
    {
        mcpHandler.setButtonCallback([this](uint8_t pin, bool pressed) {
            this->handleButtonPressed(pin);
        });

        mcpHandler.setReleaseCallback([this](uint8_t pin, bool released) {
            this->handleButtonReleased(pin);
        });

        mcpHandler.setRotaryCallback([this](int movement) {
            this->handleRotaryMovement(movement);
        });

        indicators::getButtonLed().SetStatus(ControlBoardWorkingStatus::Idle);
    }

    void ControlBoard::handleButtonPressed(uint8_t pin)
    {
        ESP_LOGI(TAG, "Button pressed on pin %u", pin);
        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::doingWork);
        spiPintActiveBitMap |= (1 << pin);
        indicators::getSpiLedDriver().setLed(pin, true);

        if (buttonActions[pin]) {
            actions::actionResponse result = buttonActions[pin]->execute(false);
            responseProcessor->process(result);
        }
    }

    void ControlBoard::handleButtonReleased(uint8_t pin)
    {
        ESP_LOGI(TAG, "Button released on pin %u", pin);
        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::Idle);

        if (buttonActions[pin]) {
            actions::actionResponse result = buttonActions[pin]->execute(true);
            responseProcessor->process(result);

            if (!result.KeepLedActive) {
                spiPintActiveBitMap &= ~(1 << pin);
                indicators::getSpiLedDriver().setLed(pin, false);
            }
        }
    }

    void ControlBoard::handleRotaryMovement(int movement)
    {
        ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::doingWork);
        
        if (buttonActions[ControlBoardConfig::BTN_ROTARY_EVENT_1]) {
            actions::actionResponse result = buttonActions[ControlBoardConfig::BTN_ROTARY_EVENT_1]->execute(movement > 0);
            responseProcessor->process(result);
        }
    }

    void ControlBoard::createButtonActionMap()
    {
        buttonActions[ControlBoardConfig::BTN_POWER] = &actions::PowerButtonInstance;
        buttonActions[ControlBoardConfig::BTN_PREV_TRACK] = &actions::PreviousTrackInstance;
        buttonActions[ControlBoardConfig::BTN_NEXT_TRACK] = &actions::NextTrackInstance;
        buttonActions[ControlBoardConfig::BTN_SKIP_FORWARD] = &actions::SkipForwardInstance;
        buttonActions[ControlBoardConfig::BTN_SKIP_BACK] = &actions::SkipBackInstance;
        buttonActions[ControlBoardConfig::BTN_PLAY_PAUSE] = &actions::PlayPauseInstance;
        buttonActions[ControlBoardConfig::BTN_STOP] = &actions::StopInstance;
        buttonActions[ControlBoardConfig::BTN_PREV_MENU] = &actions::PreviousMenuInstance;
        buttonActions[ControlBoardConfig::BTN_NEXT_MENU] = &actions::NextMenuInstance;
        buttonActions[ControlBoardConfig::BTN_MENU_SELECT] = &actions::MenuSelectInstance;
        buttonActions[ControlBoardConfig::BTN_TOGGLE_DAC] = &actions::ToggleDacInstance;
        buttonActions[ControlBoardConfig::BTN_TOGGLE_DISPLAY] = &actions::ToggleDisplayInstance;
        buttonActions[ControlBoardConfig::BTN_TOGGLE_METER] = &actions::ToggleMeterDisplayInstance;
        buttonActions[ControlBoardConfig::BTN_ROTARY_EVENT_1] = &actions::RotaryEventInstance;
        buttonActions[ControlBoardConfig::BTN_ROTARY_EVENT_2] = &actions::RotaryEventInstance;
    }
    void ControlBoard::handleSerialRxMessage(const UARTMessage &msg)

    {
        ESP_LOGI(TAG, "Received UART message - Command ID: 0x%04X, Sequence: %u, Type: %u",
                 msg.command_id, msg.sequence, msg.msg_type);

        // Reset heartbeat timer on message reception
        // (This is handled by Serial class updating last_rx_time_us)

        // Check if this is a heartbeat message from RPI
        const uint16_t CMD_ID_HEARTBEAT = 0x9999;
        if (msg.command_id == CMD_ID_HEARTBEAT) {
            ESP_LOGI(TAG, "Heartbeat message received from RPI");
            if (responseProcessor) {
                responseProcessor->onHeartbeatReceived();
            }
            return;
        }

        // Process other messages through the action processor
        if (responseProcessor) {
            // Convert UART message to action response or handle as needed
            // This depends on your message format and action system
            ESP_LOGI(TAG, "Processing UART message through action processor");
        }
    }
}

