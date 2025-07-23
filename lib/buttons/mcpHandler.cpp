#include "mcpHandler.hpp"
#include "esp_check.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c.h>


namespace buttons
{

    static const char *TAG = "MCP";

    // Lookup table for rotary encoder transitions
    static constexpr int8_t ROTARY_TABLE[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0};

    MCPInputHandler::MCPInputHandler(uint8_t address, i2c_port_t port, gpio_num_t intPin)
        : i2cAddr(address), i2cPort(port), interruptPin(intPin), prevState(0xFFFF), rotaryLast(0), ticksToWait(pdMS_TO_TICKS(50))
    {
    }

    esp_err_t MCPInputHandler::begin()
    {
        ESP_LOGI(TAG, "Initializing I2C on port %d", i2cPort);

        // 1. Configure I2C hardware
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = GPIO_NUM_16;
        conf.scl_io_num = GPIO_NUM_15;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.clk_flags = 0;
        conf.master.clk_speed = 50000; // Set I2C clock speed to 100kHz

         ESP_RETURN_ON_ERROR(i2c_param_config(i2cPort, &conf), TAG, "I2C config failed");
         ESP_RETURN_ON_ERROR(i2c_driver_install(i2cPort, I2C_MODE_MASTER, 0, 0, 0), TAG, "I2C install failed");

        ESP_LOGI(TAG, "Configuring MCP23017 registers");

        // 2. Configure GPIO direction and pull-ups (IODIRx and GPPUx)
        writeRegisterPair(0x00, 0xFF, 0xFF); // IODIRA/B: inputs
        writeRegisterPair(0x0C, 0xFF, 0xFF); // GPPUA/B: pull-ups on

        // 3. Enable interrupt-on-change on all pins
        writeRegisterPair(0x04, 0xFF, 0xFF); // GPINTENA/B: interrupt on change
        writeRegisterPair(0x08, 0x00, 0x00); // INTCONA/B: compare to previous
        writeRegisterPair(0x0A, 0x00, 0x00); // DEFVALA/B: don't care
        writeRegister(0x0A, 0x00);           // IOCON: default
        writeRegister(0x0B, 0b01000000);     // IOCON mirror, open-drain

        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << interruptPin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,
        };

        // 4. Configure interrupt pin
        ESP_LOGI(TAG, "Configuring interrupt pin %d", interruptPin);
        gpio_config(&io_conf);

        if (gpio_install_isr_service(0) != ESP_OK)
        {
            ESP_LOGW(TAG, "ISR service already installed, skipping");
        };

        ESP_LOGI(TAG, "Adding ISR handler for pin %d", interruptPin);
        ESP_RETURN_ON_ERROR(gpio_isr_handler_add(interruptPin, [](void *arg) -> void
                                                 { static_cast<MCPInputHandler *>(arg)->handleInterrupt(); }, this),
                            TAG, "ISR add failed");


        return ESP_OK;
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
        uint16_t current = readGPIO16();

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
