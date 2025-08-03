#include "activeLed.hpp"
#include "esp_log.h"

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY      5000 // 5kHz PWM

namespace indicators {

ActiveLed::ActiveLed(gpio_num_t pin,ledc_channel_t channel) : pin(pin), currentStatus(ControlBoardWorkingStatus::Idle)
{
    ledc_timer_config_t ledc_timer = {};
        ledc_timer.speed_mode       = LEDC_MODE;
        ledc_timer.duty_resolution  = LEDC_DUTY_RES;
        ledc_timer.timer_num        = LEDC_TIMER;
        ledc_timer.freq_hz          = LEDC_FREQUENCY;
        ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
    
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {};
        ledc_channel.channel    = channel;
        ledc_channel.duty       = 0;
        ledc_channel.gpio_num   = pin;
        ledc_channel.speed_mode = LEDC_MODE;
        ledc_channel.hpoint     = 0;
        ledc_channel.timer_sel  = LEDC_TIMER;
    
    ledc_channel_config(&ledc_channel);

    blinkTimer = xTimerCreate("LedBlinkTimer", pdMS_TO_TICKS(1000), pdTRUE, this, TimerCallback);
}

ActiveLed::~ActiveLed()
{
    if (blinkTimer) {
        xTimerStop(blinkTimer, 0);
        xTimerDelete(blinkTimer, 0);
    }
    stopBreatheEffect();
}

void ActiveLed::SetStatus(ControlBoardWorkingStatus newStatus)
{
    currentStatus = newStatus;
    ledOn = true;

    stopBreatheEffect();
    xTimerStop(blinkTimer, 0);

    switch (currentStatus) {
    case ControlBoardWorkingStatus::sleeping:
        startBreatheEffect();
        break;
    case ControlBoardWorkingStatus::Active:
        updateDuty(4096);
        break;
    case ControlBoardWorkingStatus::doingWork:
    case ControlBoardWorkingStatus::Idle:
    case ControlBoardWorkingStatus::MaintenanceMode:
        xTimerChangePeriod(blinkTimer, pdMS_TO_TICKS(getBlinkInterval(currentStatus)), 0);
        xTimerStart(blinkTimer, 0);
        break;
    default:
        updateDuty(0);
        break;
    }
}

void ActiveLed::TimerCallback(TimerHandle_t xTimer)
{
    auto *self = static_cast<ActiveLed *>(pvTimerGetTimerID(xTimer));
    self->handleBlink();
}

void ActiveLed::handleBlink()
{
    ledOn = !ledOn;
    if (ledOn)
        updateDuty(getBlinkDuty(currentStatus));
    else
        updateDuty(0);
}

void ActiveLed::updateDuty(uint32_t duty)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

uint32_t ActiveLed::getBlinkInterval(ControlBoardWorkingStatus status)
{
    switch (status) {
    case ControlBoardWorkingStatus::doingWork: return 500;
    case ControlBoardWorkingStatus::Idle: return 2000;
    case ControlBoardWorkingStatus::MaintenanceMode: return 5000;
    default: return 1000;
    }
}

uint32_t ActiveLed::getBlinkDuty(ControlBoardWorkingStatus status)
{
    switch (status) {
    case ControlBoardWorkingStatus::doingWork: return 4096;
    case ControlBoardWorkingStatus::Idle: return 2048;
    case ControlBoardWorkingStatus::MaintenanceMode: return 512;
    default: return 0;
    }
}


void ActiveLed::startBreatheEffect()
{
    xTaskCreate(BreatheTask, "LedBreatheTask", 2048, this, 5, &breatheTaskHandle);
}

void ActiveLed::stopBreatheEffect()
{
    if (breatheTaskHandle) {
        vTaskDelete(breatheTaskHandle);
        breatheTaskHandle = nullptr;
    }
}

void ActiveLed::BreatheTask(void *pvParameter)
{
    auto *self = static_cast<ActiveLed *>(pvParameter);

    const int maxDuty = 1024;
    const int step = 16;
    const int delayMs = 30;

    while (true) {
        // Fade in
        for (int duty = 0; duty <= maxDuty; duty += step) {
            self->updateDuty(duty);
            vTaskDelay(pdMS_TO_TICKS(delayMs));
        }
        // Fade out
        for (int duty = maxDuty; duty >= 0; duty -= step) {
            self->updateDuty(duty);
            vTaskDelay(pdMS_TO_TICKS(delayMs));
        }
    }
}

} // namespace indicators
