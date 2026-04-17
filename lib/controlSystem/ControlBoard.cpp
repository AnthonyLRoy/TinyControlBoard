#include "ControlBoard.hpp"
#include "led_manager.hpp"
#include "protocol/uartProtocol.hpp"
#include "actionProcessor.hpp"
#include "serial.hpp"
#include "spi.hpp"
#include <inttypes.h>

namespace controlSystem
{
    static const char *spTag = "CONTROL_BOARD";
    static constexpr uint16_t kLegacyHeartbeatCommandId = 0x9999;

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

        mpSerialHandler->startHeartbeatMonitor(board::timing::kHeartbeatTimeoutMs, [this]() {
            ESP_LOGE(spTag, "Heartbeat timeout: No data received from Raspberry Pi within %" PRIu32 " ms",
                     board::timing::kHeartbeatTimeoutMs);
            // Notify action processor that heartbeat timeout occurred (RPI is offline)
            if (mpResponseProcessor) {
                mpResponseProcessor->handleHeartbeatTimeout();
            }
        });

        // Initialize SPI and action processor

        mpResponseProcessor = std::make_unique<ActionProcessor>(*mpSerialHandler, *mpRelays);

        // Setup hardware components
        if (!setupRelays()) {
            return false;
        }

        if (!setupMcpHandler()) {
            return false;
        }

        setupMcpCallbacks();

        if (!setupSerial()) {
            return false;
        }

        createButtonActionMap();

        ESP_LOGI(spTag, "ControlBoard init complete.");
        ESP_LOGI(spTag, "Transitioning Power LED to Sleep state...");
        
        vTaskDelay(pdMS_TO_TICKS(board::timing::kInitDelayMs));

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
        mpResponseProcessor.reset();
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

        indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);

        return true;
    }

    bool ControlBoard::setupSerial()
    {
        ESP_LOGI(spTag, "Initializing serial...");
        bool ok = mpSerialHandler->initUart(board::serial::kPort,
                           board::serial::kBaudRate,
                           board::serial::kTxPin,
                           board::serial::kRxPin,
                           board::serial::kBufferSize,
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
        esp_err_t err = mMcpHandler.begin(board::i2c::kSdaPin,
                                         board::i2c::kSclPin,
                                         board::i2c::kInterruptPin);
        if (err != ESP_OK)
        {
            ESP_LOGE(spTag, "Failed MCPHandler begin: %d", err);
            return false;
        }
        mMcpHandler.setTimeout(board::i2c::kMcpTimeoutMs);
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

        indicators::getButtonStatusLed().setStatus(ControlBoardWorkingStatus::SolidIdle);
    }
    /// @brief todo modify some commands to activate on release for timed button presses
    /// @param buttonPressedId 
    void ControlBoard::handleButtonPressed(uint8_t buttonPressedId)
    {
        ESP_LOGI(spTag, "Button pressed on pin %u", buttonPressedId);
        indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::doingWork);
        if (buttonPressedId > 0)
        {
            indicators::getSpiLedDriver().setLed(buttonPressedId - 1, true);
        }

        if (buttonPressedId >= board::buttons::kCount || !mpResponseProcessor)
        {
            return;
        }

        if (mpButtonActions[buttonPressedId]) {
            actions::ActionResponse result = mpButtonActions[buttonPressedId]->execute(true);
            mpResponseProcessor->process(result);
        }
    }

    void ControlBoard::handleButtonReleased(uint8_t buttonReleasedId)
    {
        ESP_LOGI(spTag, "Button released on pin %u", buttonReleasedId);
        indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);

        if (buttonReleasedId >= board::buttons::kCount || !mpResponseProcessor)
        {
            return;
        }

        if (mpButtonActions[buttonReleasedId]) {
            actions::ActionResponse result = mpButtonActions[buttonReleasedId]->execute(false);
            mpResponseProcessor->process(result);
            if (!result.keepLedActive && buttonReleasedId > 0) {
                indicators::getSpiLedDriver().setLed(buttonReleasedId - 1, false);
            }
        }
    }

    void ControlBoard::handleRotaryMovement(int direction)
    {
        ESP_LOGI(spTag, "Rotary movement: %s", (direction > 0 ? "RIGHT" : "LEFT"));
        indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::doingWork);
        // this if statement assumes both left and right rotary events are handled by the same action, only a parameter changes1 for right 2 for left, why is this seperate from Handle button pressed and released? To lazy to refactor now
        // and this is c++ not c#sharp after all, An every time i try to use references i get lost in pointer land, so sue me
        if (!mpResponseProcessor)
        {
            return;
        }

        if (mpButtonActions[board::buttons::kRotaryEventLeft]) {
            actions::ActionResponse result = mpButtonActions[board::buttons::kRotaryEventLeft]->execute(direction > 0);
            mpResponseProcessor->process(result);
        }
        indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);
    }

    void ControlBoard::createButtonActionMap()
    {
        mpButtonActions[board::buttons::kPower] = &actions::PowerButtonInstance;
        mpButtonActions[board::buttons::kPrevTrack] = &actions::PreviousTrackInstance;
        mpButtonActions[board::buttons::kNextTrack] = &actions::NextTrackInstance;
        mpButtonActions[board::buttons::kSkipForward] = &actions::SkipForwardInstance;
        mpButtonActions[board::buttons::kSkipBack] = &actions::SkipBackInstance;
        mpButtonActions[board::buttons::kPlayPause] = &actions::PlayPauseInstance;
        mpButtonActions[board::buttons::kStop] = &actions::StopInstance;
        mpButtonActions[board::buttons::kCover] = &actions::CoverViewInstance;
        mpButtonActions[board::buttons::kNextMenu] = &actions::NextMenuInstance;
        mpButtonActions[board::buttons::kMenuSelect] = &actions::MenuSelectInstance;
        mpButtonActions[board::buttons::kToggleDac] = &actions::ToggleDacInstance;
        mpButtonActions[board::buttons::kToggleDisplay] = &actions::ToggleDisplayInstance;
        mpButtonActions[board::buttons::kToggleMeter] = &actions::ToggleMeterDisplayInstance;
        mpButtonActions[board::buttons::kRotaryEventLeft] = &actions::RotaryEventInstance;
        mpButtonActions[board::buttons::kRotaryEventRight] = &actions::RotaryEventInstance;
        mpButtonActions[board::buttons::kCycleBrightness] = &actions::CycleBrightnessInstance;
    }
    void ControlBoard::handleSerialRxMessage(const UartMessage &rMsg)

    {
        ESP_LOGI(spTag, "Received UART message - Command ID: 0x%04X, Sequence: %u, Type: %u",
                 rMsg.commandId, rMsg.sequence, rMsg.msgType);

        // Reset heartbeat timer on message reception
        // (This is handled by Serial class updating last_rx_time_us)

        // Check if this is a heartbeat message from RPI
        if (rMsg.commandId == CMD_SYS_HEARTBEAT || rMsg.commandId == kLegacyHeartbeatCommandId) {
            if (rMsg.commandId == kLegacyHeartbeatCommandId) {
                ESP_LOGW(spTag, "Received legacy heartbeat command 0x%04X; update the RPI heartbeat sender to CMD_SYS_HEARTBEAT (0x%04X)",
                         rMsg.commandId, CMD_SYS_HEARTBEAT);
            }
            if (mpResponseProcessor) {
                mpResponseProcessor->handleHeartbeatReceived();
            }
            return;
        }

        if (mpResponseProcessor) {
            ESP_LOGI(spTag, "Processing UART message through action processor (cmd=0x%04X)", rMsg.commandId);
        }
    }
}

