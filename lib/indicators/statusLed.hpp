#pragma once

#include "indicators/activityStatus.hpp"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <atomic>

namespace indicators {

class StatusLed {
public:
    // Creates a status LED controller bound to a specific GPIO and PWM channel.
    StatusLed(gpio_num_t pin,
              ledc_channel_t channel,
              uint32_t idleDuty = 8191 / 4,
              ControlBoardWorkingStatus defaultStatus = ControlBoardWorkingStatus::Idle);
    // Releases the LED timer and background task when the status LED is destroyed.
    ~StatusLed();

    // Changes the status used by the LED state machine and updates the output pattern.
    void setStatus(ControlBoardWorkingStatus newStatus);
    // Sends a status update to the LED queue for asynchronous processing by the task.
    void sendStatus(ControlBoardWorkingStatus status);
    /// Updates the duty applied in SolidIdle state. Thread-safe; takes effect
    /// immediately if the LED is currently in SolidIdle.
    // Updates the idle brightness of the LED without changing its current operating mode.
    void setIdleDuty(uint32_t duty);
    // Initializes the PWM timer and starts the LED task that handles state transitions.
    void init();
private:
    gpio_num_t m_pin;
    ledc_channel_t m_channel;
    std::atomic<uint32_t> m_idleDuty;
    ControlBoardWorkingStatus m_defaultStatus;
    std::atomic<ControlBoardWorkingStatus> m_currentStatus;
    bool m_ledOn = true;
    QueueHandle_t mp_statusQueue = nullptr;
    TimerHandle_t mp_blinkTimer = nullptr;
    std::atomic<bool> m_stopLedTask{false};

    TaskHandle_t mp_ledTaskHandle = nullptr;
    static void runLedTask(void *p_param);
    void updateDuty(uint32_t duty);
    static void handleTimer(TimerHandle_t timerHandle);

    uint32_t getBlinkDuty(ControlBoardWorkingStatus status);
};

} // namespace indicators