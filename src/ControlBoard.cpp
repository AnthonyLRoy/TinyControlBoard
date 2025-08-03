// ControlBoard.cpp
#include "ControlBoard.hpp"
#include "led_Manager.hpp"

#include "uart_protocol.hpp"

namespace controlSystem
{
    static const char *TAG = "CONTROL_BOARD";

    void ControlBoard::init()
    {
        ESP_LOGI(TAG, "Initializing Control Board...");
        indicators::getPowerLed().setState(ControlBoardState::Standby);

        SetupRelays();
        SetupMCPHandler();
        SetupMCPCallbacks();
        SetupSerial();
        SetupSPI();
        SetupButtonActions();
    }

    void ControlBoard::SetupRelays()
    {
        relays::StandardRelay::init(PIN_RELAY_SCREEN);
        relays::StandardRelay::init(PIN_RELAY_DAC);
        relays::StandardRelay::init(PIN_RELAY_MAINS);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_2);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_3);
        relays::StandardRelay::init(PIN_RELAY_GENERAL_4);

        relays::StandardRelay::setRelayState(PIN_RELAY_SCREEN, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_DAC, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_MAINS, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_2, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_3, false);
        relays::StandardRelay::setRelayState(PIN_RELAY_GENERAL_4, false);

        indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);
    }

    void ControlBoard::SetupSPI()
    {
        spi2.init(PIN_SPI_DATA, PIN_SPI_CLK, 1);
        indicators::getButtonLed().SetStatus(ControlBoardWorkingStatus::Active);
    }

    void ControlBoard::SetupSerial()
    {
        ESP_LOGI(TAG, "Initializing Serial Port");
        if (!serialHandler.init_uart(UART_NUM, 9600, PIN_SERIAL_TX, PIN_SERIAL_RX, 256, UART_PARITY_DISABLE, UART_STOP_BITS_1, UART_HW_FLOWCTRL_DISABLE))
        {
            ESP_LOGE(TAG, "Failed to initialize UART");
        }
        else
        {
            ESP_LOGI(TAG, "UART Initialized Successfully");
        }
    }

    void ControlBoard::SetupMCPHandler()
    {
        ESP_LOGI(TAG, "Initializing MCPInputHandler");
        ESP_ERROR_CHECK(mcpHandler.begin(PIN_I2C_SDA, PIN_I2C_SCL, PIN_I2C_INT));
        mcpHandler.setTimeout(10);
        mcpHandler.I2CEnable(true);
        mcpHandler.scanner();
        mcpHandler.dumpRegisters();
        ESP_LOGI(TAG, "MCPInputHandler Setup Complete");
    }

    void ControlBoard::SetupMCPCallbacks()
    {
        mcpHandler.setButtonCallback([this](uint8_t pin, bool pressed)
        {
            ESP_LOGI(TAG, "Pin %u %s", pin, pressed ? "PRESSED" : "RELEASED");
            if (pressed && buttonActions[pin])
            {
                indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
                buttonActions[pin]->execute();
            }
        });

        mcpHandler.setReleaseCallback([this](uint8_t pin, bool released)
        {
            ESP_LOGI(TAG, "Pin %u RELEASED", pin);
            indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);
            if (buttonActions[pin])
            {
             actions::actionResponse result =   buttonActions[pin]->execute();
            }
        });

        mcpHandler.setRotaryCallback([](int movement)
        {
            ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
            indicators::getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
        });
    }

    void ControlBoard::SetupButtonActions()
    {
        buttonActions[0] = &actions::toggleTrackInstance;
        buttonActions[1] = &actions::volumeUpInstance;
    }
} // namespace controlSystem
