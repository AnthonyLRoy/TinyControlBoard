
#include "main.h"
#include "ControlBoard.hpp"
#include "spi.hpp"

using namespace buttons;
using namespace indicators;
using namespace controlSystem;
using namespace serialBus;

#define GPPIO_NUM_SCDL GPIO_NUM_15
#define GPIO_NUM_SDA GPIO_NUM_16
#define GPIO_NUM_INTERRUPT GPIO_NUM_18
#define MCP_ADDRESS 0x20 // Default I2C address for MCP23017

#define UART_NUM UART_NUM_2
#define PIN_SERIAL_TX GPIO_NUM_2
#define PIN_SERIAL_RX GPIO_NUM_1
#define SERIAL_BUFFER_SIZE 1024

#define SPI_DATA GPIO_NUM_10
#define SPI_CLK  GPIO_NUM_12
#define SPI_LATCH GPIO_NUM_14


static const char *TAG = "CONTROL_BOARD";

ControlBoard controlBoardInstance = ControlBoard();
MCPInputHandler mcpHandler(MCP_ADDRESS, I2C_NUM_0); // MCP23017 handler
spibus::SPI spi2(SPI2_HOST);

Serial serialHandler;                               // Serial handler

extern "C" void app_main(void)
{
    static main myApp;
    nvs_flash_init();
    myApp.run();
}

void main::run()
{
    PerformStartupTasks();
    while (true)
    {
        static int64_t lastActionTime = esp_timer_get_time();
        int64_t now = esp_timer_get_time();

        if ((now - lastActionTime) > 5000000)
        { // 5 seconds
            getActiveLed().SetStatus(ControlBoardWorkingStatus::sleeping);
        }
        getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork); 
        vTaskDelay(pdMS_TO_TICKS(5000));
        getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle); 
        UARTMessage msg;
        msg.src_app = APP_ESP32;
        msg.msg_type = MSG_COMMAND;
        msg.sequence = 1;
        msg.command_id = CMD_NEXT_TRACK;
        msg.params[0] = 1;
        msg.params[1] = 20;

        uint8_t tx_buffer[UART_PACKET_SIZE];
        serialize_message(msg, tx_buffer);
        uart_write_bytes(UART_NUM, (const char *)tx_buffer, sizeof(msg));
        spi2.send(0xFFFF);
        spi2.pulse_latch(SPI_LATCH);

        vTaskDelay(pdMS_TO_TICKS(100));
        
        ESP_LOGI(TAG, "Current LED Status: 0x%02X", mcpHandler.readRegister(0X13)); // Read the current status of the LED register
        getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork); 
        
        spi2.send(0xFFFF);
        spi2.pulse_latch(SPI_LATCH);

    }
}

void SetupControlBoard()
{
    controlBoardInstance.init(); // Initialize control board
    ESP_LOGI(TAG, "Control Board Initialized");
}

void SetupSpiPort()
{
    spi2.init(SPI_DATA,SPI_CLK,1);
    getButtonLed().SetStatus(ControlBoardWorkingStatus::Active);
}

/// @brief  Initialize the button handler
/// @details This function sets up the MCPInputHandler for handling button inputs.  
/// It initializes the MCP23017 I2C device, sets the timeout for I2C commands, enables the I2C bus,
/// and scans the I2C bus for connected devices. It also dumps the registers of the MCP23017 for debugging purposes.
/// @note  This function should be called after the control board is initialized.
/// @return void
/// @see MCPInputHandler::begin(), MCPInputHandler::setTimeout(), MCPInputHandler::I2CEnable(), MCPInputHandler::scanner(), MCPInputHandler::dumpRegisters()
/// @warning Ensure that the GPIO pins for SDA, SCL, and interrupt are correctly defined
void setupButtonHandler()
{
    // Initialize MCPInputHandler

    ESP_LOGI(TAG, "Initializing MCPInputHandler");
    ESP_ERROR_CHECK(mcpHandler.begin(GPIO_NUM_SDA, GPPIO_NUM_SCDL, GPIO_NUM_INTERRUPT)); // Initialize MCP23017 with SDA and SCL pins

    ESP_LOGI(TAG, "MCPInputHandler Initialized");
    mcpHandler.setTimeout(10);  // Set timeout for I2C commands
    mcpHandler.I2CEnable(true); // Enable I2C bus
    mcpHandler.scanner();
    mcpHandler.dumpRegisters();

    ESP_LOGI(TAG, "MCPInputHandler Setup Complete");
}

/// @brief 
/// @details This function sets up the button callbacks for the MCPInputHandler.
/// It defines the actions to take when a button is pressed, released, or when the rotary encoder is moved.
void SetupButtonCallbacks()
{
    mcpHandler.setButtonCallback([](uint8_t pin, bool pressed)
                                 {
        ESP_LOGI(TAG, "Pin %u %s", pin, pressed ? "PRESSED" : "RELEASED");

        if (pressed) {
            getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
        } });

    mcpHandler.setReleaseCallback([](uint8_t pin, bool released)
                                  {
        ESP_LOGI(TAG, "Pin %u RELEASED", pin);
        getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle); });

    mcpHandler.setRotaryCallback([](int movement)
                                 {
        ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
        getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork); });
}

void SetupSerialPort()
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
void main::PerformStartupTasks()
{

    vTaskDelay(pdMS_TO_TICKS(5000)); // Delay to allow system to stabilize
    SetupControlBoard();             // Initialize control board and peripherals
    setupButtonHandler();            // Initialize button handler
    SetupButtonCallbacks();          // Set up button callbacks
    SetupSerialPort();               // Initialize serial port
    SetupSpiPort();
}

// public ot()
// {

//     controlBoardInstance.init(); // Initialize control board
//     ESP_LOGI(TAG, "Starting Control Board Application");
//     vTaskDelay(pdMS_TO_TICKS(8000)); // Delay to allow system to stabilize
//     ESP_LOGI(TAG, "starting MCP23017 test");
//     MCPInputHandler mcpHandler(0x20, I2C_NUM_0, GPIO_NUM_18);

//     mcpHandler.setTimeout(10); // e.g., timeout = 1000 ms

//     ESP_LOGI(TAG, "Starting test...");
//     ESP_ERROR_CHECK(mcpHandler.begin());

//     ESP_LOGI(TAG, "MCP23017 initialized successfully");
//     mcpHandler.setTimeout(10); // Set timeout for I2C commands
//     mcpHandler.scanner();      // Optional: scan I2C bus for devices
//     mcpHandler.testConnection(0x20, 1000);
//     // Set initial LED state to Idle
//     getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle);

//     // Button press callback
//     mcpHandler.setButtonCallback([](uint8_t pin, bool pressed)
//                                  {
//         ESP_LOGI(TAG, "Pin %u %s", pin, pressed ? "PRESSED" : "RELEASED");

//         // Visual feedback: set LED to "doing work" if pressed
//         if (pressed) {
//             getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork);
//         } });

//     // Button release callback
//     mcpHandler.setReleaseCallback([](uint8_t pin, bool released)
//                                   {
//         ESP_LOGI(TAG, "Pin %u RELEASED", pin);
//         getActiveLed().SetStatus(ControlBoardWorkingStatus::Idle); });

//     // Rotary encoder movement
//     mcpHandler.setRotaryCallback([](int movement)
//                                  {
//         ESP_LOGI(TAG, "Rotary movement: %s", (movement > 0 ? "RIGHT" : "LEFT"));
//         getActiveLed().SetStatus(ControlBoardWorkingStatus::doingWork); });

//     // Optional idle/sleep LED indication if no activity
//     while (true)
//     {
//         static int64_t lastActionTime = esp_timer_get_time();
//         int64_t now = esp_timer_get_time();

//         if ((now - lastActionTime) > 5000000)
//         { // 5 seconds
//             getActiveLed().SetStatus(ControlBoardWorkingStatus::sleeping);
//         }

//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }
