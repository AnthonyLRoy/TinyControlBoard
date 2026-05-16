//this code is based on the mcphandler code from  the the internet but modified to fit the needs of this project. 
//It is used to handle the input from the mcp23018 io expander and convert it to button presses and rotary encoder movements.



#include "input/buttons/mcpInputHandler.hpp"

namespace buttons {

static const char *spTag = "MCP             ";

McpInputHandler::McpInputHandler(uint8_t address, i2c_port_t port)
        : mI2cAddr(address),
            mI2cPort(port),
            mInterruptPin(GPIO_NUM_NC),
            mPrevState(0xFFFF),
            mRotaryLast(0),
            mTicksToWait(pdMS_TO_TICKS(50)),
            mpInterruptTaskHandle(nullptr) {}

esp_err_t McpInputHandler::begin(gpio_num_t sda, gpio_num_t scl, gpio_num_t intPin) {
    mInterruptPin = intPin;

    ESP_RETURN_ON_ERROR(initI2cBus(sda, scl), spTag, "I2C init failed");
    ESP_RETURN_ON_ERROR(initMcp23018(), spTag, "MCP init failed");
    ESP_RETURN_ON_ERROR(initInterruptPin(), spTag, "Interrupt pin setup failed");

    createInterruptTask();
    clearInitialInterrupts();

    ESP_LOGI(spTag, "MCP23018 initialized successfully");
    return ESP_OK;
}

void McpInputHandler::setButtonCallback(std::function<void(uint8_t, bool)> cb) { mButtonCallback = cb; }
void McpInputHandler::setReleaseCallback(std::function<void(uint8_t, bool)> cb) { mReleaseCallback = cb; }
void McpInputHandler::setRotaryCallback(std::function<void(int)> cb) { mRotaryCallback = cb; }
void McpInputHandler::setTimeout(uint32_t ms) { mTicksToWait = pdMS_TO_TICKS(ms); }

void McpInputHandler::enableI2c(bool enable) {
    gpio_set_level(PIN_I2C_ENABLE, enable ? 1 : 0);
    ESP_LOGI(spTag, "I2C %s", enable ? "enabled" : "disabled");
}
#ifdef DEBUG_MCP_SCAN

void McpInputHandler::dumpRegisters() const {
    for (uint8_t reg = 0x00; reg <= 0x15; ++reg) {
        uint8_t val = readRegister(reg);
        ESP_LOGI(spTag, "Reg 0x%02X = 0x%02X", reg, val);
    }
}

#endif

#ifdef DEBUG_MCP_SCAN
// Scans the I2C bus for devices and logs their addresses when found. Useful for debugging connectivity issues with the MCP23018.
void McpInputHandler::scanI2c() const {
    int found = 0;

    for (uint8_t addr = 0x03; addr < 0x78; ++addr) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(mI2cPort, cmd, pdMS_TO_TICKS(20));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(spTag, "Device at 0x%02X", addr);
            ++found;
        }
    }

    if (!found) {
        ESP_LOGW(spTag, "No I2C devices found");
    }
}

#endif

esp_err_t McpInputHandler::initI2cBus(gpio_num_t sda, gpio_num_t scl) {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda;
    conf.scl_io_num = scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_CLK_SPEED_HZ;

    ESP_RETURN_ON_ERROR(i2c_param_config(mI2cPort, &conf), spTag, "I2C config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(mI2cPort, I2C_MODE_MASTER, 0, 0, 0), spTag, "I2C install failed");

    gpio_set_direction(PIN_I2C_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_I2C_ENABLE, 1);
    return ESP_OK;
}

esp_err_t McpInputHandler::initMcp23018() {
    writeRegisterPair(MCP_IODIRA, ALL_INPUTS, ALL_INPUTS);
    writeRegisterPair(MCP_GPPUA, ALL_INPUTS, ALL_INPUTS);
    writeRegisterPair(MCP_GPINTENA, ALL_INPUTS, ALL_INPUTS);
    writeRegisterPair(MCP_INTCONA, 0x00, 0x00);
    writeRegister(MCP_IOCON, 0b01000100);
    return ESP_OK;
}

esp_err_t McpInputHandler::initInterruptPin() {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << mInterruptPin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), spTag, "Interrupt pin config failed");

    esp_err_t isr_err = gpio_install_isr_service(0);
    if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) return isr_err;

    gpio_isr_handler_add(mInterruptPin, McpInputHandler::gpioIsr, this);
    return ESP_OK;
}

void McpInputHandler::createInterruptTask() {
    xTaskCreate([](void *arg) {
        static_cast<McpInputHandler*>(arg)->runInterruptTaskLoop();
    }, "mcp_int_task", 4096, this, 10, &mpInterruptTaskHandle);
}

void McpInputHandler::clearInitialInterrupts() {
    readRegister(MCP_GPIOA);
    readRegister(MCP_GPIOB);
}

void IRAM_ATTR McpInputHandler::gpioIsr(void *pArg) {
    auto *pSelf = static_cast<McpInputHandler*>(pArg);
    BaseType_t higherPriorityWoken = pdFALSE;
    vTaskNotifyGiveFromISR(pSelf->mpInterruptTaskHandle, &higherPriorityWoken);
    portYIELD_FROM_ISR(higherPriorityWoken);
}

void McpInputHandler::runInterruptTaskLoop() {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        handleInterrupt();
    }
}

void McpInputHandler::handleInterrupt() {
    uint8_t intfA = readRegister(MCP_INTFA);
    uint8_t intfB = readRegister(MCP_INTFB);
    uint8_t intcapA = readRegister(MCP_INTCAPA);
    uint8_t intcapB = readRegister(MCP_INTCAPB);
    ESP_LOGI(spTag, "...............Handling interrupt.......................");
    ESP_LOGI(spTag, "INTFA=0x%02X INTFB=0x%02X INTCAPA=0x%02X INTCAPB=0x%02X", intfA, intfB, intcapA, intcapB);

    uint8_t gpioa = readRegister(MCP_GPIOA);
    uint8_t gpiob = readRegister(MCP_GPIOB);
    uint16_t current = (gpiob << 8) | gpioa;

    uint16_t changed = current ^ mPrevState;
    const uint16_t rotaryMask = (1u << ROTARY_A_PIN) | (1u << ROTARY_B_PIN);
    if (changed & rotaryMask) {
        decodeRotary(current);
        changed &= ~rotaryMask;
    }

    while (changed) {
        uint8_t pinIndex = __builtin_ctz(changed);
        changed &= changed - 1;

        bool isHigh = (current >> pinIndex) & 1;

        if (!isHigh && mButtonCallback)
            mButtonCallback(pinIndex, true);
        else if (isHigh && mReleaseCallback)
            mReleaseCallback(pinIndex, true);
    }

    mPrevState = current;
}

void McpInputHandler::decodeRotary(uint16_t state) {
    ESP_LOGI(spTag, "Decoding rotary with state: 0x%04X", state);
    uint8_t a = !(state & (1 << ROTARY_A_PIN));
    uint8_t b = !(state & (1 << ROTARY_B_PIN));
    uint8_t rotaryNow = (mRotaryLast << 2) | (a << 1) | b;

    int8_t move = msRotaryTable[rotaryNow & 0x0F];
    if (move && mRotaryCallback) mRotaryCallback(move);

    mRotaryLast = (a << 1) | b;
}

esp_err_t McpInputHandler::i2cWrite(const uint8_t *pData, size_t len) const {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mI2cAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, pData, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(mI2cPort, cmd, mTicksToWait);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t McpInputHandler::i2cWriteRead(uint8_t reg, uint8_t *pData, size_t len) const {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mI2cAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mI2cAddr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, pData, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(mI2cPort, cmd, mTicksToWait);
    i2c_cmd_link_delete(cmd);
    return ret;
}

uint8_t McpInputHandler::readRegister(uint8_t reg) const {
    uint8_t val = 0;
    i2cWriteRead(reg, &val, 1);
    return val;
}

uint16_t McpInputHandler::readGpio16() const {
    uint8_t data[2] = {};
    i2cWriteRead(MCP_GPIOA, data, 2);
    return (data[1] << 8) | data[0];
}

void McpInputHandler::writeRegister(uint8_t reg, uint8_t val) {
    uint8_t buf[] = {reg, val};
    i2cWrite(buf, sizeof(buf));
}

void McpInputHandler::writeRegisterPair(uint8_t baseReg, uint8_t a, uint8_t b) {
    uint8_t buf[] = {baseReg, a, b};
    i2cWrite(buf, sizeof(buf));
}

} // namespace buttons