#include "activeLed.hpp"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"

using namespace indicators;

static const char *TAG = "ActiveLed";

static constexpr uint32_t MAX_DUTY = 8191; // 13-bit resolution

ActiveLed::ActiveLed(gpio_num_t pin, ledc_channel_t channel)
    : pin(pin),
      channel(channel),
      currentStatus(ControlBoardWorkingStatus::Idle),
      ledOn(false),
      statusQueue(nullptr),
      blinkTimer(nullptr),
      breatheTaskHandle(nullptr),
      ledTaskHandle(nullptr)
{
    init();
}

ActiveLed::~ActiveLed()
{
    if (ledTaskHandle)
        vTaskDelete(ledTaskHandle);
    if (blinkTimer)
        xTimerDelete(blinkTimer, portMAX_DELAY);
    if (breatheTaskHandle)
        vTaskDelete(breatheTaskHandle);
    if (statusQueue)
        vQueueDelete(statusQueue);
}

void ActiveLed::init()
{
    ledc_timer_config_t timerConfig = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};

    ESP_ERROR_CHECK(ledc_timer_config(&timerConfig));

    ledc_channel_config_t channelConfig = {};
    channelConfig.gpio_num = pin;
    channelConfig.speed_mode = LEDC_LOW_SPEED_MODE;
    channelConfig.channel = channel;
    channelConfig.intr_type = LEDC_INTR_DISABLE;
    channelConfig.timer_sel = LEDC_TIMER_0;
    channelConfig.duty = 0;
    channelConfig.hpoint = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&channelConfig));

    statusQueue = xQueueCreate(1, sizeof(ControlBoardWorkingStatus));

    blinkTimer = xTimerCreate("BlinkTimer", pdMS_TO_TICKS(100), pdTRUE, this, TimerCallback);

    xTaskCreate(ledTask, "LED_Task", 4096, this, 5, &ledTaskHandle);
}

void ActiveLed::SetStatus(ControlBoardWorkingStatus newStatus)
{
    sendStatus(newStatus);
}

void ActiveLed::sendStatus(ControlBoardWorkingStatus status)
{
    if (statusQueue)
    {
        xQueueOverwrite(statusQueue, &status);
    }

    ESP_LOGI(TAG, "Status sent: %d", static_cast<int>(status));

    if (ledTaskHandle)
    {
        xTaskNotifyGive(ledTaskHandle);
    }
    else
    {
        ESP_LOGE(TAG, "LED Task not initialized, cannot send status");
    }
}

void ActiveLed::ledTask(void *param)
{
    auto *self = static_cast<ActiveLed *>(param);

    ControlBoardWorkingStatus receivedStatus;
    ESP_LOGI(TAG, "LED Task started on pin %d", self->pin);
    for (;;)
    {
        if (xQueueReceive(self->statusQueue, &receivedStatus, portMAX_DELAY))
        {
            self->currentStatus = receivedStatus;
            ESP_LOGI(TAG, "LED status updated to %d", static_cast<int>(receivedStatus));
            ESP_LOGI(TAG, "Handling status change for pin %d", self->pin);
            // Stop any previous effect
            xTimerStop(self->blinkTimer, 0);
            self->stopBreatheEffect();

            switch (receivedStatus)
            {
            case ControlBoardWorkingStatus::doingWork:
            case ControlBoardWorkingStatus::Idle:
            case ControlBoardWorkingStatus::MaintenanceMode:
            case ControlBoardWorkingStatus::Active:
                // Blink at the appropriate interval and brightness
                xTimerChangePeriod(
                    self->blinkTimer,
                    pdMS_TO_TICKS(self->getBlinkInterval(receivedStatus)),
                    0);
                xTimerStart(self->blinkTimer, 0);
                ESP_LOGI(TAG, "Blinking at interval: %lu ms on pin %d", self->getBlinkInterval(receivedStatus), self->pin);
                break;

            case ControlBoardWorkingStatus::sleeping:
                self->startBreatheEffect();
                break;
            }
        }
    }
}

void ActiveLed::updateDuty(uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void ActiveLed::TimerCallback(TimerHandle_t xTimer)
{
    auto *self = static_cast<ActiveLed *>(pvTimerGetTimerID(xTimer));
    self->ledOn = !self->ledOn;
    uint32_t duty = self->ledOn
                        ? self->getBlinkDuty(self->currentStatus)
                        : 0;
    self->updateDuty(duty);
}
void ActiveLed::startBreatheEffect()
{
    xTaskCreate(BreatheTask, "BreatheTask", 2048, this, 5, &breatheTaskHandle);
}

void ActiveLed::stopBreatheEffect()
{
    if (breatheTaskHandle)
    {
        vTaskDelete(breatheTaskHandle);
        breatheTaskHandle = nullptr;
    }
}

void ActiveLed::BreatheTask(void *pvParameter)
{
    auto *self = static_cast<ActiveLed *>(pvParameter);
    uint32_t duty = 0;
    bool increasing = true;

    while (true)
    {
        self->updateDuty(duty);

        if (increasing)
        {
            duty += 64;
            if (duty >= MAX_DUTY)
                increasing = false;
        }
        else
        {
            duty -= 64;
            if (duty == 0)
                increasing = true;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

uint32_t ActiveLed::getBlinkInterval(ControlBoardWorkingStatus status)
{
    switch (status)
    {
    case ControlBoardWorkingStatus::Idle:
        return 1000; // 1 second period
    case ControlBoardWorkingStatus::MaintenanceMode:
        return 3000;
    case ControlBoardWorkingStatus::Active:
        return 500;
    case ControlBoardWorkingStatus::doingWork:
        return 100; // Solid ON
    default:
        return 1000;
    }
}

uint32_t ActiveLed::getBlinkDuty(ControlBoardWorkingStatus status)
{
    switch (status)
    {
    case ControlBoardWorkingStatus::Idle:
        return MAX_DUTY / 4; // Dim blink for idle
    case ControlBoardWorkingStatus::MaintenanceMode:
        return MAX_DUTY / 2;
    case ControlBoardWorkingStatus::Active:
        return MAX_DUTY;
    default:
        return MAX_DUTY;
    }
}
