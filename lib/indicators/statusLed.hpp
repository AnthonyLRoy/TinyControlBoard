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
    StatusLed(gpio_num_t pin,
              ledc_channel_t channel,
              uint32_t idleDuty = 8191 / 4,
              ControlBoardWorkingStatus defaultStatus = ControlBoardWorkingStatus::Idle);
    ~StatusLed();

    void setStatus(ControlBoardWorkingStatus newStatus);
    void sendStatus(ControlBoardWorkingStatus status);
    /// Updates the duty applied in SolidIdle state. Thread-safe; takes effect
    /// immediately if the LED is currently in SolidIdle.
    void setIdleDuty(uint32_t duty);
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
    TaskHandle_t mp_breatheTaskHandle = nullptr;
    std::atomic<bool> m_stopLedTask{false};
    std::atomic<bool> m_stopBreatheTask{false};

    TaskHandle_t mp_ledTaskHandle = nullptr;
    static void runLedTask(void *p_param);
    void updateDuty(uint32_t duty);
    static void handleTimer(TimerHandle_t timerHandle);
    static void runBreatheTask(void *p_parameter);
    void startBreatheEffect();
    void stopBreatheEffect();

    uint32_t getBlinkInterval(ControlBoardWorkingStatus status);
    uint32_t getBlinkDuty(ControlBoardWorkingStatus status);
};

} // namespace indicators