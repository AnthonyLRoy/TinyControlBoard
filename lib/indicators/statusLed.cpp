#include "statusLed.hpp"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"

using namespace indicators;

static const char *spTag = "Status_Led      ";

static constexpr uint32_t MAX_DUTY = 8191;
static constexpr uint32_t BREATHE_STEP = 64;
static constexpr uint32_t BREATHE_DELAY_MS = 20;

StatusLed::StatusLed(gpio_num_t pin,
                     ledc_channel_t channel,
                     uint32_t idleDuty,
                     ControlBoardWorkingStatus defaultStatus)
        : mPin(pin),
            mChannel(channel),
            mIdleDuty(idleDuty),
            mDefaultStatus(defaultStatus),
            mCurrentStatus(defaultStatus),
            mLedOn(false),
            mpStatusQueue(nullptr),
            mpBlinkTimer(nullptr),
            mpBreatheTaskHandle(nullptr),
            mpLedTaskHandle(nullptr)
{
    init();
}

StatusLed::~StatusLed()
{
    if (mpLedTaskHandle)
        vTaskDelete(mpLedTaskHandle);
    if (mpBlinkTimer)
        xTimerDelete(mpBlinkTimer, portMAX_DELAY);
    if (mpBreatheTaskHandle)
        vTaskDelete(mpBreatheTaskHandle);
    if (mpStatusQueue)
        vQueueDelete(mpStatusQueue);
}

void StatusLed::init()
{
    ledc_timer_config_t timerConfig = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};

    ESP_ERROR_CHECK(ledc_timer_config(&timerConfig));

    ledc_channel_config_t channelConfig = {};
    channelConfig.gpio_num = mPin;
    channelConfig.speed_mode = LEDC_LOW_SPEED_MODE;
    channelConfig.channel = mChannel;
    channelConfig.intr_type = LEDC_INTR_DISABLE;
    channelConfig.timer_sel = LEDC_TIMER_0;
    channelConfig.duty = 0;
    channelConfig.hpoint = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&channelConfig));

    if (mDefaultStatus == ControlBoardWorkingStatus::SolidIdle)
    {
        mLedOn = true;
        updateDuty(mIdleDuty);
    }

    mpStatusQueue = xQueueCreate(1, sizeof(ControlBoardWorkingStatus));

    mpBlinkTimer = xTimerCreate("BlinkTimer", pdMS_TO_TICKS(100), pdTRUE, this, handleTimer);

    xTaskCreate(runLedTask, "LED_Task", 4096, this, 5, &mpLedTaskHandle);
}

void StatusLed::setStatus(ControlBoardWorkingStatus newStatus)
{
    sendStatus(newStatus);
}

void StatusLed::sendStatus(ControlBoardWorkingStatus status)
{
    if (mpStatusQueue)
    {
        xQueueOverwrite(mpStatusQueue, &status);
    }

    ESP_LOGI(spTag, "Status sent: %d", static_cast<int>(status));

    if (mpLedTaskHandle)
    {
        xTaskNotifyGive(mpLedTaskHandle);
    }
    else
    {
        ESP_LOGE(spTag, "LED Task not initialized, cannot send status");
    }
}

void StatusLed::runLedTask(void *pParam)
{
    auto *pSelf = static_cast<StatusLed *>(pParam);

    ControlBoardWorkingStatus receivedStatus;
    ESP_LOGI(spTag, "LED Task started on pin %d", pSelf->mPin);
    for (;;)
    {
        if (xQueueReceive(pSelf->mpStatusQueue, &receivedStatus, portMAX_DELAY))
        {
            pSelf->mCurrentStatus = receivedStatus;
            ESP_LOGI(spTag, "LED status updated to %d", static_cast<int>(receivedStatus));
            ESP_LOGI(spTag, "Handling status change for pin %d", pSelf->mPin);
            xTimerStop(pSelf->mpBlinkTimer, 0);
            pSelf->stopBreatheEffect();
            pSelf->mLedOn = false;
            pSelf->updateDuty(0);

            switch (receivedStatus)
            {
            case ControlBoardWorkingStatus::doingWork:
                pSelf->mLedOn = true;
                pSelf->updateDuty(MAX_DUTY);
                break;
            case ControlBoardWorkingStatus::SolidIdle:
                pSelf->mLedOn = true;
                pSelf->updateDuty(pSelf->mIdleDuty);
                break;
            case ControlBoardWorkingStatus::Idle:
            case ControlBoardWorkingStatus::MaintenanceMode:
            case ControlBoardWorkingStatus::Active:
                xTimerChangePeriod(
                    pSelf->mpBlinkTimer,
                    pdMS_TO_TICKS(pSelf->getBlinkInterval(receivedStatus)),
                    0);
                xTimerStart(pSelf->mpBlinkTimer, 0);
                ESP_LOGI(spTag, "Blinking at interval: %lu ms on pin %d", pSelf->getBlinkInterval(receivedStatus), pSelf->mPin);
                break;

            case ControlBoardWorkingStatus::sleeping:
                pSelf->startBreatheEffect();
                break;
            }
        }
    }
}

void StatusLed::updateDuty(uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, mChannel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, mChannel);
}

void StatusLed::handleTimer(TimerHandle_t timerHandle)
{
    auto *pSelf = static_cast<StatusLed *>(pvTimerGetTimerID(timerHandle));
    pSelf->mLedOn = !pSelf->mLedOn;
    uint32_t duty = pSelf->mLedOn
                        ? pSelf->getBlinkDuty(pSelf->mCurrentStatus)
                        : 0;
    pSelf->updateDuty(duty);
}

void StatusLed::startBreatheEffect()
{
    if (mpBreatheTaskHandle)
    {
        return;
    }
    xTaskCreate(runBreatheTask, "BreatheTask", 2048, this, 5, &mpBreatheTaskHandle);
}

void StatusLed::stopBreatheEffect()
{
    if (mpBreatheTaskHandle)
    {
        vTaskDelete(mpBreatheTaskHandle);
        mpBreatheTaskHandle = nullptr;
        updateDuty(0);
    }
}

void StatusLed::runBreatheTask(void *pParameter)
{
    auto *pSelf = static_cast<StatusLed *>(pParameter);
    uint32_t duty = 0;
    bool increasing = true;

    while (true)
    {
        pSelf->updateDuty(duty);

        if (increasing)
        {
            if (duty + BREATHE_STEP >= MAX_DUTY)
            {
                duty = MAX_DUTY;
                increasing = false;
            }
            else
            {
                duty += BREATHE_STEP;
            }
        }
        else
        {
            if (duty <= BREATHE_STEP)
            {
                duty = 0;
                increasing = true;
            }
            else
            {
                duty -= BREATHE_STEP;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BREATHE_DELAY_MS));
    }
}

uint32_t StatusLed::getBlinkInterval(ControlBoardWorkingStatus status)
{
    switch (status)
    {
    case ControlBoardWorkingStatus::Idle:
        return 1000;
    case ControlBoardWorkingStatus::MaintenanceMode:
        return 3000;
    case ControlBoardWorkingStatus::Active:
        return 500;
    case ControlBoardWorkingStatus::doingWork:
    case ControlBoardWorkingStatus::SolidIdle:
        return 100;
    default:
        return 1000;
    }
}

uint32_t StatusLed::getBlinkDuty(ControlBoardWorkingStatus status)
{
    switch (status)
    {
    case ControlBoardWorkingStatus::SolidIdle:
    case ControlBoardWorkingStatus::Idle:
        return mIdleDuty;
    case ControlBoardWorkingStatus::MaintenanceMode:
        return MAX_DUTY / 2;
    case ControlBoardWorkingStatus::Active:
        return MAX_DUTY;
    default:
        return MAX_DUTY;
    }
}