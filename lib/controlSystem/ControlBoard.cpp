#include "ControlBoard.hpp"
#include "led_Manager.hpp"
#include "uart_protocol.hpp"
#include "actionProcessor.hpp"
#include "serial.hpp"
#include "spi.hpp"

namespace controlSystem
{
    static const char* TAG = "CONTROL_BOARD";

    bool ControlBoard::init()
    {
        ESP_LOGI(TAG, "Starting ControlBoard init...");

        indicators::getPowerLed().setState(ControlBoardState::Standby);
        uint16_t  spiButtonStatus = 0;
        serialHandler = new serialBus::Serial();
        relays = new relays::StandardRelay();
        spi = new spibus::SPI(SPI2_HOST);

        responseProcessor = new actionProcessor(*serialHandler, *relays, *spi);

        if (!setupRelays()) return false;
        if (!setupMCPHandler()) return false;
        
        setupMCPCallbacks();

        if (!setupSerial()) return false;
        if (!setupSPI()) return false;

        setupButtonActions();

        ESP_LOGI(TAG, "ControlBoard init complete.");
        return true;
    }

    void ControlBoard::deinit()
    {
        // Optional, for completeness or if you want soft reset support
        delete responseProcessor;
        delete spi;
        delete relays;
        delete serialHandler;

        responseProcessor = nullptr;
        spi = nullptr;
        relays = nullptr;
        serialHandler = nullptr;
    }

    bool ControlBoard::setupRelays()
    {
        ESP_LOGI(TAG, "Setting up relays...");
        relays::StandardRelay::init(PIN_RELAY_SCREEN);
        relays::StandardRelay::init(PIN_RELAY_RPI);
        relays::StandardRelay::init(PIN_RELAY_DAC);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_2);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_3);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_4);

        relays::StandardRelay::setRelayState(PIN_RELAY_SCREEN, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_RPI, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_DAC, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_2, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_3, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_4, false);

        indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);

        return true; 
    }

    bool ControlBoard::setupSerial()
    {
        ESP_LOGI(TAG, "Initializing serial...");
        bool ok = serialHandler->init_uart(UART_NUM, UART_BOARD_RATE, PIN_SERIAL_TX, PIN_SERIAL_RX, 256,
                                           UART_PARITY_DISABLE, UART_STOP_BITS_1, UART_HW_FLOWCTRL_DISABLE);
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
        indicators::getButtonLed().SetStatus(ControlBoardWorkingStatus::Active);
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
        mcpHandler.scanner();
        mcpHandler.dumpRegisters();
        ESP_LOGI(TAG, "MCP Handler ready.");
        return true;
    }

    void ControlBoard::setupMCPCallbacks()
    {
        indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
        mcpHandler.setButtonCallback([this](uint8_t pin, bool pressed)
        {
            spi->send(0xFF);
            ESP_LOGI(TAG, "Pin %u %s", pin, pressed ? "PRESSED" : "RELEASED");
            if (pressed && buttonActions[pin])
            {
                actions::actionResponse result = buttonActions[pin]->execute(false);
                responseProcessor->process(result);
            }
        });

        mcpHandler.setReleaseCallback([this](uint8_t pin, bool released)
        {
            spi->send(0x00);
            ESP_LOGI(TAG, "Pin %u RELEASED", pin);
            indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);
            if (buttonActions[pin] && released)
            {
                actions::actionResponse result = buttonActions[pin]->execute(true);
            }
        });

        mcpHandler.setRotaryCallback([](int movement)
        {
            ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
            indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
        });
    }

    void ControlBoard::setupButtonActions()
    {
        buttonActions[0] = &actions::toggleTrackInstance;
        buttonActions[1] = &actions::volumeUpInstance;
        buttonActions[2] = &actions::PowerButtonInstance;
    }
}
