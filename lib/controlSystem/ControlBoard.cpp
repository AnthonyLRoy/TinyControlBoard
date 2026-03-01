#include "ControlBoard.hpp"
#include "led_Manager.hpp"
#include "uart_protocol.hpp"
#include "actionProcessor.hpp"
#include "serial.hpp"
#include "spi.hpp"
#include <inttypes.h>

namespace controlSystem
{
    static const char *spTag = "CONTROL_BOARD";

    bool ControlBoard::init()
    {
        ESP_LOGI(spTag, "Starting ControlBoard init...");

        // Set initial power state
        indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);
        //make the monitor brightness start at 50% so it's not blinding when we turn it on
        indicators::getMonitorBrightnessController().changeBrightnessLevel(5);

        // Initialize serial handler and heartbeat monitor
        mpSerialHandler = &serialBus::Serial::getInstance();
        mpRelays = &relays::StandardRelay::getInstance();
        
        // Register UART RX callback
        mpSerialHandler->setRxCallback([this](const UartMessage &rMsg) {
            this->handleSerialRxMessage(rMsg);
        });

        mpSerialHandler->startHeartbeatMonitor(ControlBoardConfig::HEARTBEAT_TIMEOUT_MS, [this]() {
            ESP_LOGE(spTag, "Heartbeat timeout: No data received from Raspberry Pi within %" PRIu32 " ms",
                     ControlBoardConfig::HEARTBEAT_TIMEOUT_MS);
            // Notify action processor that heartbeat timeout occurred (RPI is offline)
            if (mpResponseProcessor) {
                mpResponseProcessor->handleHeartbeatTimeout();
            }
        });

        // Initialize SPI and action processor

        mpResponseProcessor = new ActionProcessor(*mpSerialHandler, *mpRelays);

        // Setup hardware components 
        if (!setupRelays()) {
            ESP_LOGE(spTag, "Failed to setup relays");
            return false;
        }

        if (!setupMcpHandler()) {
            ESP_LOGE(spTag, "Failed to setup MCP handler");
            return false;
        }

        setupMcpCallbacks();

        if (!setupSerial()) {
            ESP_LOGE(spTag, "Failed to setup serial");
            return false;
        }

        createButtonActionMap();

        ESP_LOGI(spTag, "ControlBoard init complete.");
        ESP_LOGI(spTag, "Transitioning Power LED to Sleep state...");
        
        vTaskDelay(pdMS_TO_TICKS(ControlBoardConfig::INIT_DELAY_MS));

        indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);

        return true;
    }

    void ControlBoard::deinit()
    {
        if (mpSerialHandler)
        {
            mpSerialHandler->deinitUart();
            mpSerialHandler = nullptr;
        }
        delete mpResponseProcessor;
        mpResponseProcessor = nullptr;
    }

    bool ControlBoard::setupRelays()
    {
        ESP_LOGI(spTag, "Setting up relays...");
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

        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::Idle);

        return true;
    }

    bool ControlBoard::setupSerial()
    {
        ESP_LOGI(spTag, "Initializing serial...");
        bool ok = mpSerialHandler->initUart(ControlBoardConfig::UART_NUM,
                                           ControlBoardConfig::UART_BOARD_RATE,
                                           ControlBoardConfig::PIN_SERIAL_TX,
                                           ControlBoardConfig::PIN_SERIAL_RX,
                                           256,
                                           UART_PARITY_DISABLE,
                                           UART_STOP_BITS_1,
                                           UART_HW_FLOWCTRL_DISABLE);
        if (!ok)
        {
            ESP_LOGE(spTag, "Failed to initialize UART");
            return false;
        }
        ESP_LOGI(spTag, "UART initialized successfully");
        return true;
    }


    bool ControlBoard::setupMcpHandler()
    {
        ESP_LOGI(spTag, "Initializing MCP handler...");
        esp_err_t err = mMcpHandler.begin(ControlBoardConfig::PIN_I2C_SDA,
                                         ControlBoardConfig::PIN_I2C_SCL,
                                         ControlBoardConfig::PIN_I2C_INT);
        if (err != ESP_OK)
        {
            ESP_LOGE(spTag, "Failed MCPHandler begin: %d", err);
            return false;
        }
        mMcpHandler.setTimeout(ControlBoardConfig::MCP_TIMEOUT_MS);
        mMcpHandler.enableI2c(true);
#ifdef DEBUG_MCP_SCAN
        mMcpHandler.scanI2c();
#endif
        ESP_LOGI(spTag, "MCP Handler initialized successfully.");
#ifdef DEBUG_MCP_SCAN
        mMcpHandler.dumpRegisters();
#endif
        return true;
    }

    void ControlBoard::setupMcpCallbacks()
    {
        mMcpHandler.setButtonCallback([this](uint8_t pin, bool pressed) {
            this->handleButtonPressed(pin);
        });

        mMcpHandler.setReleaseCallback([this](uint8_t pin, bool released) {
            this->handleButtonReleased(pin);
        });

        mMcpHandler.setRotaryCallback([this](int movement) {
            this->handleRotaryMovement(movement);
        });

        indicators::getButtonLed().setStatus(ControlBoardWorkingStatus::Idle);
    }
    /// @brief todo modify some commands to activate on release for timed button presses
    /// @param buttonPressedId 
    void ControlBoard::handleButtonPressed(uint8_t buttonPressedId)
    {
        ESP_LOGI(spTag, "Button pressed on pin %u", buttonPressedId);
        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::doingWork);
        indicators::getSpiLedDriver().setLed(buttonPressedId, true);

        if (mpButtonActions[buttonPressedId]) {
            actions::ActionResponse result = mpButtonActions[buttonPressedId]->execute(true);
            mpResponseProcessor->process(result);
        }
    }

    void ControlBoard::handleButtonReleased(uint8_t buttonReleasedId)
    {
        ESP_LOGI(spTag, "Button released on pin %u", buttonReleasedId);
        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::Idle);

        if (mpButtonActions[buttonReleasedId]) {
            actions::ActionResponse result = mpButtonActions[buttonReleasedId]->execute(false);
            mpResponseProcessor->process(result);
            if (!result.keepLedActive) {
                indicators::getSpiLedDriver().setLed(buttonReleasedId, false);
            }
        }
    }

    void ControlBoard::handleRotaryMovement(int direction)
    {
        ESP_LOGI(spTag, "Rotary movement: %s", (direction > 0 ? "RIGHT" : "LEFT"));
        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::doingWork);
        // this if statement assumes both left and right rotary events are handled by the same action, only a parameter changes1 for right 2 for left, why is this seperate from Handle button pressed and released? To lazy to refactor now
        // and this is c++ not c#sharp after all, An every time i try to use references i get lost in pointer land, so sue me
        if (mpButtonActions[ControlBoardConfig::BTN_ROTARY_EVENT_LEFT]) {
            actions::ActionResponse result = mpButtonActions[ControlBoardConfig::BTN_ROTARY_EVENT_LEFT]->execute(direction > 0);
            mpResponseProcessor->process(result);
        }
        indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::Idle);
    }

    void ControlBoard::createButtonActionMap()
    {
        mpButtonActions[ControlBoardConfig::BTN_POWER] = &actions::PowerButtonInstance;
        mpButtonActions[ControlBoardConfig::BTN_PREV_TRACK] = &actions::PreviousTrackInstance;
        mpButtonActions[ControlBoardConfig::BTN_NEXT_TRACK] = &actions::NextTrackInstance;
        mpButtonActions[ControlBoardConfig::BTN_SKIP_FORWARD] = &actions::SkipForwardInstance;
        mpButtonActions[ControlBoardConfig::BTN_SKIP_BACK] = &actions::SkipBackInstance;
        mpButtonActions[ControlBoardConfig::BTN_PLAY_PAUSE] = &actions::PlayPauseInstance;
        mpButtonActions[ControlBoardConfig::BTN_STOP] = &actions::StopInstance;
        mpButtonActions[ControlBoardConfig::BTN_COVER] = &actions::CoverViewInstance;
        mpButtonActions[ControlBoardConfig::BTN_NEXT_MENU] = &actions::NextMenuInstance;
        mpButtonActions[ControlBoardConfig::BTN_MENU_SELECT] = &actions::MenuSelectInstance;
        mpButtonActions[ControlBoardConfig::BTN_TOGGLE_DAC] = &actions::ToggleDacInstance;
        mpButtonActions[ControlBoardConfig::BTN_TOGGLE_DISPLAY] = &actions::ToggleDisplayInstance;
        mpButtonActions[ControlBoardConfig::BTN_TOGGLE_METER] = &actions::ToggleMeterDisplayInstance;
        mpButtonActions[ControlBoardConfig::BTN_ROTARY_EVENT_LEFT] = &actions::RotaryEventInstance;
        mpButtonActions[ControlBoardConfig::BTN_ROTARY_EVENT_RIGHT] = &actions::RotaryEventInstance;
        mpButtonActions[ControlBoardConfig::BTN_CYCLE_BRIGHTNESS] = &actions::CycleBrightnessInstance;
    }
    void ControlBoard::handleSerialRxMessage(const UartMessage &rMsg)

    {
        //ESP_LOGI(TAG, "Received UART message - Command ID: 0x%04X, Sequence: %u, Type: %u",
        //         msg.command_id, msg.sequence, msg.msg_type);

        // Reset heartbeat timer on message reception
        // (This is handled by Serial class updating last_rx_time_us)

        // Check if this is a heartbeat message from RPI
        const uint16_t CMD_ID_HEARTBEAT = 0x9999;  //todo move this to message definitions
        if (rMsg.commandId == CMD_ID_HEARTBEAT) {
            //ESP_LOGI(TAG, "Heartbeat message received from RPI");
            if (mpResponseProcessor) {
                mpResponseProcessor->handleHeartbeatReceived();
            }
            return;
        }

        // can't rememebr why this is here should remove serves no purpose 
        if (mpResponseProcessor) {
            // Convert UART message to action response or handle as needed
            // This depends on your message format and action system
            ESP_LOGI(spTag, "Processing UART message through action processor");
        }
    }
}

