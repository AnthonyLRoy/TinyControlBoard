#pragma once

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"

enum class ControlBoardWorkingStatus {
    doingWork,
    Idle,
    sleeping,
    MaintenanceMode
};

namespace indicators {

class ActiveLed {
public:
    ActiveLed(gpio_num_t pin);
    ~ActiveLed();

    void SetStatus(ControlBoardWorkingStatus newStatus);

private:
    void updateDuty(uint32_t duty);
    static void TimerCallback(TimerHandle_t xTimer);
    void handleBlink();

    static void BreatheTask(void *pvParameter);
    void startBreatheEffect();
    void stopBreatheEffect();

    gpio_num_t pin;
    ControlBoardWorkingStatus currentStatus;
    bool ledOn = true;

    TimerHandle_t blinkTimer = nullptr;
    TaskHandle_t breatheTaskHandle = nullptr;

    uint32_t getBlinkInterval(ControlBoardWorkingStatus status);
    uint32_t getBlinkDuty(ControlBoardWorkingStatus status);
};

} // namespace indicators
