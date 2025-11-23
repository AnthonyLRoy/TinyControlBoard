#include "mcpInputHandler.hpp"


namespace buttons {

static const char *TAG = "MCP";

// Constructor
MCPInputHandler::MCPInputHandler(uint8_t address, i2c_port_t port)
    : i2cAddr(address),
      i2cPort(port),
      interruptPin(GPIO_NUM_NC),
      prevState(0xFFFF),
      rotaryLast(0),
      ticksToWait(pdMS_TO_TICKS(50)),
      interruptTaskHandle(nullptr) {}

// Public API
esp_err_t MCPInputHandler::begin(gpio_num_t sda, gpio_num_t scl, gpio_num_t intPin) {
    interruptPin = intPin;

    ESP_RETURN_ON_ERROR(initI2CBus(sda, scl), TAG, "I2C init failed");
    ESP_RETURN_ON_ERROR(initMCP23018(), TAG, "MCP init failed");
    ESP_RETURN_ON_ERROR(initInterruptPin(), TAG, "Interrupt pin setup failed");

    createInterruptTask();
    clearInitialInterrupts();

    ESP_LOGI(TAG, "MCP23018 initialized successfully");
    return ESP_OK;
}

void MCPInputHandler::setButtonCallback(std::function<void(uint8_t, bool)> cb) { buttonCallback = cb; }
void MCPInputHandler::setReleaseCallback(std::function<void(uint8_t, bool)> cb) { releaseCallback = cb; }
void MCPInputHandler::setRotaryCallback(std::function<void(int)> cb) { rotaryCallback = cb; }
void MCPInputHandler::setTimeout(uint32_t ms) { ticksToWait = pdMS_TO_TICKS(ms); }

void MCPInputHandler::I2CEnable(bool enable) {
    gpio_set_level(PIN_I2C_ENABLE, enable ? 1 : 0);
    ESP_LOGI(TAG, "I2C %s", enable ? "enabled" : "disabled");
}
#ifdef DEBUG_MCP_SCAN

void MCPInputHandler::dumpRegisters() const {
    for (uint8_t reg = 0x00; reg <= 0x15; ++reg) {
        uint8_t val = readRegister(reg);
        ESP_LOGI(TAG, "Reg 0x%02X = 0x%02X", reg, val);
    }
}

#endif

#ifdef DEBUG_MCP_SCAN

void MCPInputHandler::scanner() const {
    int found = 0;

    for (uint8_t addr = 0x03; addr < 0x78; ++addr) {
        // Create I2C command link
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        // Send command and delete link
        esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, pdMS_TO_TICKS(20));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device at 0x%02X", addr);
            ++found;
        }
    }

    if (!found) {
        ESP_LOGW(TAG, "No I2C devices found");
    }
}

#endif

// Setup helpers
esp_err_t MCPInputHandler::initI2CBus(gpio_num_t sda, gpio_num_t scl) {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda;
    conf.scl_io_num = scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_CLK_SPEED_HZ;

    ESP_RETURN_ON_ERROR(i2c_param_config(i2cPort, &conf), TAG, "I2C config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(i2cPort, I2C_MODE_MASTER, 0, 0, 0), TAG, "I2C install failed");

    gpio_set_direction(PIN_I2C_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_I2C_ENABLE, 1);
    return ESP_OK;
}

esp_err_t MCPInputHandler::initMCP23018() {
    writeRegisterPair(MCP_IODIRA, ALL_INPUTS, ALL_INPUTS);
    writeRegisterPair(MCP_GPPUA,  ALL_INPUTS, ALL_INPUTS);
    writeRegisterPair(MCP_GPINTENA, ALL_INPUTS, ALL_INPUTS);
    writeRegisterPair(MCP_INTCONA, 0x00, 0x00);
    writeRegister(MCP_IOCON, 0b01000100); // MIRROR=1, SEQOP=1
    return ESP_OK;
}

esp_err_t MCPInputHandler::initInterruptPin() {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << interruptPin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Interrupt pin config failed");

    esp_err_t isr_err = gpio_install_isr_service(0);
    if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) return isr_err;

    gpio_isr_handler_add(interruptPin, MCPInputHandler::gpioISR, this);
    return ESP_OK;
}

void MCPInputHandler::createInterruptTask() {
    xTaskCreate([](void *arg) {
        static_cast<MCPInputHandler*>(arg)->interruptTaskLoop();
    }, "mcp_int_task", 4096, this, 10, &interruptTaskHandle);
}

void MCPInputHandler::clearInitialInterrupts() {
    readRegister(MCP_GPIOA);
    readRegister(MCP_GPIOB);
}

// Interrupt handling
void IRAM_ATTR MCPInputHandler::gpioISR(void *arg) {
    auto *self = static_cast<MCPInputHandler*>(arg);
    BaseType_t higherPriorityWoken = pdFALSE;
    vTaskNotifyGiveFromISR(self->interruptTaskHandle, &higherPriorityWoken);
    portYIELD_FROM_ISR(higherPriorityWoken);
}

void MCPInputHandler::interruptTaskLoop() {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        handleInterrupt();
    }
}

void MCPInputHandler::handleInterrupt() {
    uint8_t intfA = readRegister(MCP_INTFA);
    uint8_t intfB = readRegister(MCP_INTFB);
    uint8_t intcapA = readRegister(MCP_INTCAPA);
    uint8_t intcapB = readRegister(MCP_INTCAPB);
    ESP_LOGI(TAG,    "...............Handling interrupt.......................");
    ESP_LOGI(TAG, "INTFA=0x%02X INTFB=0x%02X INTCAPA=0x%02X INTCAPB=0x%02X", intfA, intfB, intcapA, intcapB);

    uint8_t gpioa = readRegister(MCP_GPIOA);
    uint8_t gpiob = readRegister(MCP_GPIOB);
    uint16_t current = (gpiob << 8) | gpioa;


// ...existing code...
    uint16_t changed = current ^ prevState;

    // Handle rotary bits once if either changed, then remove them from the bit-scan
    const uint16_t rotaryMask = (1u << ROTARY_A_PIN) | (1u << ROTARY_B_PIN);
    if (changed & rotaryMask) {
        decodeRotary(current);
        changed &= ~rotaryMask;
    }

    while (changed) {
        uint8_t loopCntr = __builtin_ctz(changed);
        changed &= changed - 1;

        bool now = (current >> loopCntr) & 1;

        if (!now && buttonCallback)
            buttonCallback(loopCntr, true);
        else if (now && releaseCallback)
            releaseCallback(loopCntr, true);
    }

    prevState = current;
}
  // todo: do not fire on relase if not pressed before
void MCPInputHandler::decodeRotary(uint16_t state) {
    
    ESP_LOGI(TAG, "Decoding rotary with state: 0x%04X", state);
    uint8_t a = !(state & (1 << ROTARY_A_PIN));
    uint8_t b = !(state & (1 << ROTARY_B_PIN));
    uint8_t rotaryNow = (rotaryLast << 2) | (a << 1) | b;

    int8_t move = ROTARY_TABLE[rotaryNow & 0x0F];
    if (move && rotaryCallback) rotaryCallback(move);

    rotaryLast = (a << 1) | b;
}

// I2C helpers
esp_err_t MCPInputHandler::i2cWrite(const uint8_t *data, size_t len) const {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, ticksToWait);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t MCPInputHandler::i2cWriteRead(uint8_t reg, uint8_t *data, size_t len) const {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2cAddr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2cPort, cmd, ticksToWait);
    i2c_cmd_link_delete(cmd);
    return ret;
}

uint8_t MCPInputHandler::readRegister(uint8_t reg) const {
    uint8_t val = 0;
    i2cWriteRead(reg, &val, 1);
    return val;
}

uint16_t MCPInputHandler::readGPIO16() const {
    uint8_t data[2] = {};
    i2cWriteRead(MCP_GPIOA, data, 2);
    return (data[1] << 8) | data[0];
}

void MCPInputHandler::writeRegister(uint8_t reg, uint8_t val) {
    uint8_t buf[] = {reg, val};
    i2cWrite(buf, sizeof(buf));
}

void MCPInputHandler::writeRegisterPair(uint8_t baseReg, uint8_t a, uint8_t b) {
    uint8_t buf[] = {baseReg, a, b};
    i2cWrite(buf, sizeof(buf));
}

} // namespace buttons
