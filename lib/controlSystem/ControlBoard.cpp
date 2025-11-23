#include "ControlBoard.hpp"
#include "led_Manager.hpp"
#include "uart_protocol.hpp"
#include "actionProcessor.hpp"
#include "serial.hpp"
#include "spi.hpp"

namespace controlSystem
{
    static const char *TAG = "CONTROL_BOARD";

    bool ControlBoard::init()
    {
        ESP_LOGI(TAG, "Starting ControlBoard init...");

        // system just switched on from the mains switch so default  -- no histyory storage yet
        indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);

        serialHandler = &serialBus::Serial::instance();
        spi = &spibus::SPI::instance(SPI2_HOST);
        ESP_LOGW(TAG, "Creating actionProcessor...");
        responseProcessor = new actionProcessor(*serialHandler, *relays, *spi);
        ESP_LOGW(TAG, "actionProcessor created.");

        if (!setupRelays())
            return false;
        ESP_LOGW(TAG, "Relays setup complete.");
        if (!setupMCPHandler())
            return false;
        ESP_LOGW(TAG, "MCP Handler setup complete.");
        setupMCPCallbacks();
        ESP_LOGW(TAG, "MCP Callbacks setup complete.");

        if (!setupSerial())
            return false;
        if (!setupSPI())
            return false;
        ESP_LOGW(TAG, "SPI setup complete.");
        setupButtonActions();

        ESP_LOGI(TAG, "ControlBoard init complete.");
        ESP_LOGW(TAG, "Setting Power LED to Standby...");
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
        bool ok = serialHandler->init_uart(UART_NUM,
                                           UART_BOARD_RATE,
                                           PIN_SERIAL_TX,
                                           PIN_SERIAL_RX,
                                           256,
                                           UART_PARITY_DISABLE,
                                           UART_STOP_BITS_1,
                                           UART_HW_FLOWCTRL_DISABLE);
        if (!ok)
        {
            ESP_LOGE(TAG, "Failed to initialize UART");
            return false;
        }
        ESP_LOGI(TAG, "UART Initialized");
        return true;
    }

    bool ControlBoard::setupSPI()
    {
        ESP_LOGI(TAG, "Initializing SPI...");
        spi->init(PIN_SPI_DATA, PIN_SPI_CLK, 1);

        return true;
    }

    bool ControlBoard::setupMCPHandler()
    {
        ESP_LOGI(TAG, "Initializing MCP handler...");
        esp_err_t err = mcpHandler.begin(PIN_I2C_SDA, PIN_I2C_SCL, PIN_I2C_INT);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed MCPHandler begin: %d", err);
            return false;
        }
        mcpHandler.setTimeout(10);
        mcpHandler.I2CEnable(true);
#ifdef DEBUG_MCP_SCAN
        mcpHandler.scanner();
#endif
        ESP_LOGI(TAG, "MCP Handler initialized successfully.");
#ifdef DEBUG_MCP_SCAN
        mcpHandler.dumpRegisters();
#endif
        ESP_LOGI(TAG, "MCP Handler ready.");
        return true;
    }

    void ControlBoard::setupMCPCallbacks()
    {

        mcpHandler.setButtonCallback([this](uint8_t pin, bool pressed)
                                     {
            indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::doingWork);
            spiPintActiveBitMap |= (1 << pin);
             ESP_LOGI(TAG, "Pin %u  %s", pin, pressed ? "PRESSED" : "RELEASED");

            if (pressed && buttonActions[pin])
            {
                actions::actionResponse result = buttonActions[pin]->execute(false);
                responseProcessor->process(result);
            } });

        mcpHandler.setReleaseCallback([this](uint8_t pin, bool released)
                                      {
;
            ESP_LOGI(TAG, "Pin %u RELEASED", pin);
            indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::Idle);
            if (buttonActions[pin] && released)
            {
                actions::actionResponse result = buttonActions[pin]->execute(true);
                responseProcessor->process(result);
                if(!result.KeepLedActive)
                spiPintActiveBitMap &= ~(1 << pin);

            } });

        mcpHandler.setRotaryCallback([this](int movement)
                                     {
            ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
            indicators::getActiveLed().sendStatus(ControlBoardWorkingStatus::doingWork);
            actions::actionResponse result;
            if (movement > 0)
                result = buttonActions[ROTARY_A_PIN]->execute(true);
            else
                result = buttonActions[ROTARY_B_PIN]->execute(false);
            responseProcessor->process(result); });
        indicators::getButtonLed().SetStatus(ControlBoardWorkingStatus::Idle);
    }

    void ControlBoard::setupButtonActions()
    {
        buttonActions[0] = &actions::PowerButtonInstance;
        buttonActions[1] = &actions::PreviousTrackInstance;
        buttonActions[2] = &actions::NextTrackInstance;
        buttonActions[5] = &actions::PauseInstance;
        buttonActions[6] = &actions::StopInstance;
        buttonActions[3] = &actions::SkipForwardInstance;
        buttonActions[4] = &actions::SkipBackInstance;
        buttonActions[7] = &actions::PreviousMenuInstance;
        buttonActions[8] = &actions::NextMenuInstance;
        buttonActions[9] = &actions::MenuSelectInstance;
        buttonActions[12] = &actions::ToggleMeterDisplayInstance;
        buttonActions[11] = &actions::ToggleDisplayInstance;
        buttonActions[10] = &actions::ToggleDacInstance;
        buttonActions[13] = &actions::RotaryRightInstance;
        buttonActions[14] = &actions::RotaryLeftInstance;
    }
}
