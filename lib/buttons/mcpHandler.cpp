#include "mcpHandler.hpp"

#define PIN_I2C_ENABLE GPIO_NUM_17 // GPIO to enable I2C bus
namespace buttons
{

    static const char *TAG = "MCP";

    // Lookup table for rotary encoder transitions
    static constexpr int8_t ROTARY_TABLE[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0};

    MCPInputHandler::MCPInputHandler(uint8_t address, i2c_port_t port)
        : i2cAddr(address), i2cPort(port), prevState(0xFFFF), rotaryLast(0), ticksToWait(pdMS_TO_TICKS(50))
    {
    }

    /// @brief 
    /// @details This function initializes the MCPInputHandler with the specified I2C address and port.
    /// It sets up the I2C configuration, GPIO interrupt pin, and installs the ISR          
    /// service for handling interrupts from the MCP23017 device.
    /// @param sda GPIO pin for I2C SDA
    /// @param scl GPIO pin for I2C SCL
    /// @param intPin GPIO pin for the interrupt from the MCP23017
    /// @return esp_err_t ESP_OK on success, error code on failure
    /// @see i2c_param_config(), i2c_driver_install(), gpio_config(), gpio_install_isr_service(), gpio_isr_handler_add()
    /// @warning Ensure that the GPIO pins for SDA, SCL, and interrupt are correctly
    /// defined in the project configuration.
    /// @note This function should be called after the control board is initialized.
    esp_err_t MCPInputHandler::begin(gpio_num_t sda, gpio_num_t scl, gpio_num_t intPin)
    {
        this->interruptPin = intPin;

        ESP_LOGI(TAG, "Initializing I2C on port %d", i2cPort);
        ESP_LOGI(TAG, "Using I2C address 0x%02X", i2cAddr);

        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = sda;
        conf.scl_io_num = scl;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = 50000;

        ESP_RETURN_ON_ERROR(i2c_param_config(i2cPort, &conf), TAG, "I2C config failed");
        ESP_RETURN_ON_ERROR(i2c_driver_install(i2cPort, I2C_MODE_MASTER, 0, 0, 0), TAG, "I2C install failed");

        gpio_set_direction(PIN_I2C_ENABLE, GPIO_MODE_OUTPUT);
        gpio_set_level(PIN_I2C_ENABLE, 1);

        ESP_LOGI(TAG, "Configuring MCP23017");

        writeRegisterPair(0x00, 0xFF, 0xFF); // All inputs
        writeRegisterPair(0x0C, 0xFF, 0xFF); // Pull-ups
        writeRegisterPair(0x04, 0xFF, 0xFF); // Interrupt on change
        writeRegisterPair(0x08, 0x00, 0x00); // Compare to previous

        writeRegister(0x0A, 0b01000100); // MIRROR=1, SEQOP=1

        ESP_LOGI(TAG, "Configuring interrupt pin %d", interruptPin);
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << interruptPin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Interrupt pin config failed");
        ESP_LOGI(TAG, "Interrupt pin %d configured", interruptPin);
        ESP_LOGI(TAG, "Installing ISR service");
        esp_err_t isr_err = gpio_install_isr_service(0);
        if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(TAG, "ISR install failed");
            return isr_err;
        }
        ESP_LOGI(TAG, "Creating interrupt task");
        interruptTaskHandle = nullptr;
        xTaskCreate([](void *arg)
                    { static_cast<MCPInputHandler *>(arg)->interruptTaskLoop(); }, "mcp_int_task", 4096, this, 10, &interruptTaskHandle);
        ESP_LOGI(TAG, "Adding GPIO ISR handler");
        gpio_isr_handler_add(interruptPin, MCPInputHandler::gpioISR, this);
        ESP_LOGI(TAG, "MCP23017 initialized successfully");
        writeRegisterPair(0x04, 0xFF, 0xFF); // Interrupt on change

        writeRegisterPair(0x08, 0x00, 0x00); // Compare to previous

        ESP_LOGI(TAG, "Reading initial GPIO state to force clear interrupts");
        readRegister(0x12); // Read GPIOA
        readRegister(0x13); // Read GPIOB
        return ESP_OK;
    }

    /// @brief
    /// @details This function handles the GPIO interrupt from the MCP23017.
    /// It reads the interrupt flags and captures the GPIO state, then processes the button presses and
    /// rotary encoder movements.
    /// @note This function is called from the ISR and should not block or take too long.
    /// @see readRegister(), readGPIO16(), handleInterrupt()
    /// @warning Ensure that the GPIO ISR is correctly configured to call this function.
    /// @return 
    void IRAM_ATTR MCPInputHandler::gpioISR(void *arg)
    {

        MCPInputHandler *self = static_cast<MCPInputHandler *>(arg);
        BaseType_t higherPriorityWoken = pdFALSE;
        vTaskNotifyGiveFromISR(self->interruptTaskHandle, &higherPriorityWoken);
        portYIELD_FROM_ISR(higherPriorityWoken);
    }

    /// @brief 
    /// @details This function runs in a separate task and waits for notifications from the GPIO ISR.
    /// It calls handleInterrupt() to process the GPIO state and button presses.
    /// @note This function should be created as a FreeRTOS task.
    void MCPInputHandler::interruptTaskLoop()
    {
        while (true)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for ISR to notify
            handleInterrupt();                       // Safe to call I2C here
        }
    }

    uint8_t MCPInputHandler::readRegister(uint8_t reg)
    {
        uint8_t val = 0;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_start(cmd); // Repeated start
        i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_READ, true);
        i2c_master_read_byte(cmd, &val, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2cPort, cmd, ticksToWait);
        i2c_cmd_link_delete(cmd);
        return val;
    }
    void MCPInputHandler::dumpRegisters()
    {
        ESP_LOGI(TAG, "Reading all MCP23018 registers:");
        for (uint8_t reg = 0x00; reg <= 0x15; ++reg)
        {
            uint8_t val = readRegister(reg);
            ESP_LOGI(TAG, "Reg 0x%02X = 0x%02X", reg, val);
        }
    }
    void MCPInputHandler::I2CEnable(bool enable)
    {
        gpio_set_level(PIN_I2C_ENABLE, enable ? 1 : 0);
        ESP_LOGI(TAG, "I2C %s", enable ? "enabled" : "disabled");
    }

    void MCPInputHandler::writeRegister(uint8_t reg, uint8_t val)
    {
        uint8_t buf[] = {reg, val};
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, buf, sizeof(buf), true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2cPort, cmd, ticksToWait);
        i2c_cmd_link_delete(cmd);
    }

    void MCPInputHandler::writeRegisterPair(uint8_t baseReg, uint8_t a, uint8_t b)
    {
        uint8_t buf[] = {baseReg, a, b};
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, buf, sizeof(buf), true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2cPort, cmd, ticksToWait);
        i2c_cmd_link_delete(cmd);
    }

    uint16_t MCPInputHandler::readGPIO16()
    {

        ESP_LOGI(TAG, "Reading GPIO16 state");
        uint8_t data[2] = {0};
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, 0x12, true); // GPIOA
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2cPort, cmd, ticksToWait);
        i2c_cmd_link_delete(cmd);

        return (data[1] << 8) | data[0];
    }

    void MCPInputHandler::handleInterrupt()
    {

        uint8_t intfA = readRegister(0x0E); // INTFA
        uint8_t intfB = readRegister(0x0F); // INTFB

        ESP_LOGI(TAG, "INTFA = 0x%02X", intfA);
        ESP_LOGI(TAG, "INTFB = 0x%02X", intfB);

        uint8_t intcapA = readRegister(0x10); // INTCAPA
        uint8_t intcapB = readRegister(0x11); // INTCAPB

        ESP_LOGI(TAG, "INTCAPA = 0x%02X", intcapA);
        ESP_LOGI(TAG, "INTCAPB = 0x%02X", intcapB);

        ESP_LOGI(TAG, "Interrupt received on pin %d", interruptPin);

        // Explicitly read GPIOA and GPIOB to clear INTFA/INTFB
        uint8_t gpioa = readRegister(0x12); // GPIOA
        uint8_t gpiob = readRegister(0x13); // GPIOB
        uint16_t current = (gpiob << 8) | gpioa;

        for (int i = 0; i < 16; ++i)
        {
            bool now = (current >> i) & 1;
            bool before = (prevState >> i) & 1;

            if (now != before)
            {
                if (!now && buttonCallback)
                {
                    buttonCallback(i, true); // Button pressed
                }
                else if (now && releaseCallback)
                {
                    releaseCallback(i, true); // Button released
                }
            }
        }

        decodeRotary(current); // Optional: if using rotary encoders
        prevState = current;
    }
    void MCPInputHandler::decodeRotary(uint16_t state)
    {
        uint8_t a = !(state & (1 << 14));
        uint8_t b = !(state & (1 << 15));
        uint8_t rotaryNow = (rotaryLast << 2) | (a << 1) | b;

        int8_t move = ROTARY_TABLE[rotaryNow & 0x0F];
        if (move != 0 && rotaryCallback)
        {
            rotaryCallback(move);
        }

        rotaryLast = (a << 1) | b;
    }

    void MCPInputHandler::timerCallback(TimerHandle_t xTimer)
    {
        uint32_t pin = (uint32_t)pvTimerGetTimerID(xTimer);
        MCPInputHandler *self = static_cast<MCPInputHandler *>(pvTimerGetTimerID(xTimer));
        if (self->releaseCallback)
        {
            self->releaseCallback(pin, true);
        }
    }

    void MCPInputHandler::scanner()
    {
        ESP_LOGI("I2C", "Scanning...");
        int found = 0;
        for (uint8_t addr = 0x03; addr < 0x78; ++addr)
        {
            if (testConnection(addr, 20) == ESP_OK)
            {
                ESP_LOGI("I2C", "Device at 0x%02X", addr);
                ++found;
            }
            else
            {
                ESP_LOGI("I2C", "No device at 0x%02X", addr);
            }
        }
        if (found == 0)
            ESP_LOGI("I2C", "No devices found");
    }

    esp_err_t MCPInputHandler::testConnection(uint8_t devAddr, int32_t timeout)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(i2cPort, cmd,
                                             (timeout < 0 ? ticksToWait : pdMS_TO_TICKS(timeout)));
        i2c_cmd_link_delete(cmd);
        return err;
    }

    void MCPInputHandler::setTimeout(uint32_t ms)
    {
        ticksToWait = pdMS_TO_TICKS(ms);
    }

    void MCPInputHandler::setButtonCallback(std::function<void(uint8_t, bool)> cb)
    {
        buttonCallback = cb;
    }

    void MCPInputHandler::setReleaseCallback(std::function<void(uint8_t, bool)> cb)
    {
        releaseCallback = cb;
    }

    void MCPInputHandler::setRotaryCallback(std::function<void(int)> cb)
    {
        rotaryCallback = cb;
    }

} // namespace buttons
