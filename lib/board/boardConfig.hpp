#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"

namespace board {

namespace timing {
inline constexpr uint32_t kHeartbeatTimeoutMs = 30000;
inline constexpr uint32_t kInitDelayMs = 5000;
} // namespace timing

namespace serial {
inline constexpr uart_port_t kPort = UART_NUM_2;
inline constexpr uint32_t kBaudRate = 115200;
inline constexpr gpio_num_t kTxPin = GPIO_NUM_2;
inline constexpr gpio_num_t kRxPin = GPIO_NUM_1;
inline constexpr std::size_t kBufferSize = 256;
inline constexpr gpio_num_t kRpiDataReadyPin = GPIO_NUM_42;
inline constexpr gpio_num_t kEsp32DataReadyPin = GPIO_NUM_41;
} // namespace serial

namespace i2c {
inline constexpr gpio_num_t kSclPin = GPIO_NUM_15;
inline constexpr gpio_num_t kSdaPin = GPIO_NUM_16;
inline constexpr gpio_num_t kInterruptPin = GPIO_NUM_18;
inline constexpr gpio_num_t kEnablePin = GPIO_NUM_17;
inline constexpr uint8_t kMcpAddress = 0x20;
inline constexpr int kMcpTimeoutMs = 10;
inline constexpr uint32_t kClockSpeedHz = 50000;
} // namespace i2c

namespace relays {
inline constexpr gpio_num_t kScreenPower = GPIO_NUM_13;
inline constexpr gpio_num_t kDacPower = GPIO_NUM_12;
inline constexpr gpio_num_t kRpiPower = GPIO_NUM_11;
inline constexpr gpio_num_t kOutputStagePower = GPIO_NUM_9;
inline constexpr gpio_num_t kProtoDacEnabled = GPIO_NUM_10;
inline constexpr gpio_num_t kGeneral1 = GPIO_NUM_47;
inline constexpr gpio_num_t kGeneral2 = GPIO_NUM_39;
} // namespace relays

namespace indicators {
inline constexpr gpio_num_t kAppActiveLed = GPIO_NUM_3;
inline constexpr gpio_num_t kAppStandbyLed = GPIO_NUM_4;
inline constexpr gpio_num_t kWorkingStatusLed = GPIO_NUM_48;
inline constexpr gpio_num_t kButtonLedPwmPin = GPIO_NUM_21;
inline constexpr uint32_t   kButtonLedDefaultDuty = (8191 * 8) / 10;
inline constexpr gpio_num_t kMonitorBrightness = GPIO_NUM_43;
inline constexpr gpio_num_t kSpiData = GPIO_NUM_7;
inline constexpr gpio_num_t kSpiClock = GPIO_NUM_6;
inline constexpr gpio_num_t kSpiLatch = GPIO_NUM_5;
} // namespace indicators

namespace buttons {
inline constexpr uint8_t kPower = 0;
inline constexpr uint8_t kPrevTrack = 1;
inline constexpr uint8_t kNextTrack = 2;
inline constexpr uint8_t kSkipForward = 3;
inline constexpr uint8_t kSkipBack = 4;
inline constexpr uint8_t kPlayPause = 5;
inline constexpr uint8_t kStop = 6;
inline constexpr uint8_t kCover = 7;
inline constexpr uint8_t kRepeat = 8;
inline constexpr uint8_t kToggleRandom = 9;
inline constexpr uint8_t kToggleDac = 10;
inline constexpr uint8_t kToggleDisplay = 11;
inline constexpr uint8_t kToggleMeter = 12;
inline constexpr uint8_t kRotaryEventLeft = 13;
inline constexpr uint8_t kRotaryEventRight = 14;
inline constexpr uint8_t kCycleBrightness = 15;
inline constexpr uint8_t kCount = 16;
} // namespace buttons

namespace debug {
    // Set to true to skip waiting for the RPi heartbeat during boot.
    // Useful for testing firmware locally without a connected Raspberry Pi.
    inline constexpr bool kSimulateRpiBoot = false;
} // namespace debug

} // namespace board
