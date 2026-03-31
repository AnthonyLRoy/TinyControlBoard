#pragma once

#include "indicators/activityStatus.hpp"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/queue.h"

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
    void init();
private:
    gpio_num_t mPin;
    ledc_channel_t mChannel;
    uint32_t mIdleDuty;
    ControlBoardWorkingStatus mDefaultStatus;
    ControlBoardWorkingStatus mCurrentStatus;
    bool mLedOn = true;
    QueueHandle_t mpStatusQueue = nullptr;
    TimerHandle_t mpBlinkTimer = nullptr;
    TaskHandle_t mpBreatheTaskHandle = nullptr;

    TaskHandle_t mpLedTaskHandle = nullptr;
    static void runLedTask(void *pParam);
    void updateDuty(uint32_t duty);
    static void handleTimer(TimerHandle_t timerHandle);
    static void runBreatheTask(void *pParameter);
    void startBreatheEffect();
    void stopBreatheEffect();

    uint32_t getBlinkInterval(ControlBoardWorkingStatus status);
    uint32_t getBlinkDuty(ControlBoardWorkingStatus status);
};

} // namespace indicators