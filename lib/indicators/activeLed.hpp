#pragma once

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/queue.h"
enum class ControlBoardWorkingStatus {
    doingWork,
    Idle,
    sleeping,
    MaintenanceMode,
    Active
};

namespace indicators {

class ActiveLed {
public:
    ActiveLed(gpio_num_t pin,ledc_channel_t channel);
    ~ActiveLed();

    void SetStatus(ControlBoardWorkingStatus newStatus);
    void sendStatus(ControlBoardWorkingStatus status);
    void init();
private:
    gpio_num_t pin;
    ledc_channel_t channel;
    ControlBoardWorkingStatus currentStatus;
    bool ledOn = true;
    QueueHandle_t statusQueue = nullptr;
    TimerHandle_t blinkTimer = nullptr;
    TaskHandle_t breatheTaskHandle = nullptr;

    TaskHandle_t ledTaskHandle = nullptr;
    static void ledTask(void* param);
    void updateDuty(uint32_t duty);
    static void TimerCallback(TimerHandle_t xTimer);
    void handleBlink();

    static void BreatheTask(void *pvParameter);
    void startBreatheEffect();
    void stopBreatheEffect();


    uint32_t getBlinkInterval(ControlBoardWorkingStatus status);
    uint32_t getBlinkDuty(ControlBoardWorkingStatus status);
};

} // namespace indicators
