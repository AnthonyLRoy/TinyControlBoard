
#include "mcpHandler.hpp"


namespace buttons
{

    static const char *TAG = "MCP";

    MCPInputHandler::MCPInputHandler(uint8_t address, i2c_port_t port, gpio_num_t intPin)
        : i2cAddr(address), i2cPort(port), interruptPin(intPin) {}

    void MCPInputHandler::begin()
    {
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = GPIO_NUM_16;
        conf.scl_io_num = GPIO_NUM_15;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = 100000;
        i2c_param_config(i2cPort, &conf);
        i2c_driver_install(i2cPort, conf.mode, 0, 0, 0);

        uint8_t buf[] = {0x00, 0xFF, 0xFF};
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, buf, sizeof(buf), true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);

        gpio_set_direction(interruptPin, GPIO_MODE_INPUT);
        gpio_set_intr_type(interruptPin, GPIO_INTR_NEGEDGE);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(interruptPin, [](void *arg)
                             { static_cast<MCPInputHandler *>(arg)->handleInterrupt(); }, this);
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

    uint16_t MCPInputHandler::readGPIO16()
    {
        uint8_t cmd = 0x12;
        uint8_t data[2] = {0};

        i2c_cmd_handle_t c = i2c_cmd_link_create();
        i2c_master_start(c);
        i2c_master_write_byte(c, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(c, cmd, true);
        i2c_master_start(c);
        i2c_master_write_byte(c, (i2cAddr << 1) | I2C_MASTER_READ, true);
        i2c_master_read(c, data, 2, I2C_MASTER_LAST_NACK);
        i2c_master_stop(c);
        i2c_master_cmd_begin(i2cPort, c, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(c);

        return data[1] << 8 | data[0];
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

    void MCPInputHandler::handleInterrupt()
    {
        uint16_t state = readGPIO16();

        for (int i = 0; i < 16; ++i)
        {
            bool curr = (state >> i) & 1;
            bool prev = (prevState >> i) & 1;

            if (curr != prev)
            {
                if (i == 14 || i == 15)
                    continue;

                if (curr == 0 && buttonCallback)
                {
                    buttonCallback(i, false);
                    if (timers[i])
                        xTimerDelete(timers[i], 0);
                    timers[i] = xTimerCreate("long", pdMS_TO_TICKS(1000), pdFALSE, (void *)i, timerCallback);
                    xTimerStart(timers[i], 0);
                }
                else if (curr == 1 && releaseCallback)
                {
                    xTimerStop(timers[i], 0);
                    releaseCallback(i, false);
                }
            }
        }

        // Quadrature encoder decoding using lookup table
        static const int8_t rotary_table[16] = {
            0, -1, 1, 0,
            1, 0, 0, -1,
            -1, 0, 0, 1,
            0, 1, -1, 0};

        uint8_t a = !(state & (1 << 14));
        uint8_t b = !(state & (1 << 15));
        uint8_t rotaryNow = (rotaryLast << 2) | (a << 1) | b;
        int8_t movement = rotary_table[rotaryNow & 0x0F];

        if (movement != 0 && rotaryCallback)
        {
            rotaryCallback(movement);
        }

        rotaryLast = (a << 1) | b;
        prevState = state;
    }

}
