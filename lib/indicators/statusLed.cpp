#include "statusLed.hpp"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"

using namespace indicators;

static constexpr const char *k_logTag = "Status_Led      ";

static constexpr uint32_t MAX_DUTY = 8191;
static constexpr uint32_t ACT_DUTY = 2000;
static constexpr uint32_t BLIP_ON_MS = 50;
static constexpr uint32_t BLIP_OFF_MS = 1950;
static constexpr uint32_t SLEEP_BLIP_OFF_MS = 9950;
static constexpr uint32_t PWM_FREQ_HZ = 5000;
static constexpr uint32_t BLINK_TIMER_PERIOD_MS = 100;
static constexpr uint32_t LED_TASK_STACK_SIZE = 4096;
static constexpr UBaseType_t LED_TASK_PRIORITY = 5;

StatusLed::StatusLed(gpio_num_t pin,
                     ledc_channel_t channel,
                     uint32_t idleDuty,
                     ControlBoardWorkingStatus defaultStatus)
        : m_pin(pin),
            m_channel(channel),
            m_idleDuty(idleDuty),
            m_defaultStatus(defaultStatus),
            m_currentStatus(defaultStatus),
            m_ledOn(false),
            mp_statusQueue(nullptr),
            mp_blinkTimer(nullptr),
            mp_ledTaskHandle(nullptr)
{
    init();
}

StatusLed::~StatusLed()
{
    m_stopLedTask.store(true, std::memory_order_release);

    if (mp_statusQueue)
    {
        const ControlBoardWorkingStatus wakeStatus = m_currentStatus.load();
        xQueueOverwrite(mp_statusQueue, &wakeStatus);
    }

    if (mp_ledTaskHandle && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        for (int i = 0; mp_ledTaskHandle && i < 50; ++i)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    if (mp_ledTaskHandle)
    {
        ESP_LOGW(k_logTag, "LED task did not stop in time; forcing delete");
        vTaskDelete(mp_ledTaskHandle);
        mp_ledTaskHandle = nullptr;
    }

    if (mp_blinkTimer)
        xTimerDelete(mp_blinkTimer, portMAX_DELAY);
    if (mp_statusQueue)
        vQueueDelete(mp_statusQueue);
}

void StatusLed::init()
{
    m_stopLedTask.store(false, std::memory_order_release);

    ledc_timer_config_t timerConfig = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK};

    ESP_ERROR_CHECK(ledc_timer_config(&timerConfig));

    ledc_channel_config_t channelConfig = {};
    channelConfig.gpio_num = m_pin;
    channelConfig.speed_mode = LEDC_LOW_SPEED_MODE;
    channelConfig.channel = m_channel;
    channelConfig.intr_type = LEDC_INTR_DISABLE;
    channelConfig.timer_sel = LEDC_TIMER_0;
    channelConfig.duty = 0;
    channelConfig.hpoint = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&channelConfig));

    if (m_defaultStatus == ControlBoardWorkingStatus::SolidIdle)
    {
        m_ledOn = true;
        updateDuty(m_idleDuty.load());
    }

    mp_statusQueue = xQueueCreate(1, sizeof(ControlBoardWorkingStatus));
    if (!mp_statusQueue)
    {
        ESP_LOGE(k_logTag, "Failed to create status queue");
        return;
    }

    mp_blinkTimer = xTimerCreate("BlinkTimer", pdMS_TO_TICKS(BLINK_TIMER_PERIOD_MS), pdTRUE, this, handleTimer);
    if (!mp_blinkTimer)
    {
        ESP_LOGE(k_logTag, "Failed to create blink timer");
        vQueueDelete(mp_statusQueue);
        mp_statusQueue = nullptr;
        return;
    }

    if (xTaskCreate(runLedTask, "LED_Task", LED_TASK_STACK_SIZE, this, LED_TASK_PRIORITY, &mp_ledTaskHandle) != pdPASS)
    {
        ESP_LOGE(k_logTag, "Failed to create LED task");
        xTimerDelete(mp_blinkTimer, 0);
        mp_blinkTimer = nullptr;
        vQueueDelete(mp_statusQueue);
        mp_statusQueue = nullptr;
        mp_ledTaskHandle = nullptr;
        return;
    }
}

void StatusLed::setStatus(ControlBoardWorkingStatus newStatus)
{
    sendStatus(newStatus);
}

void StatusLed::sendStatus(ControlBoardWorkingStatus status)
{
    if (mp_statusQueue)
    {
        xQueueOverwrite(mp_statusQueue, &status);
    }

    ESP_LOGI(k_logTag, "Status sent: %d", static_cast<int>(status));

    if (mp_ledTaskHandle)
    {
        xTaskNotifyGive(mp_ledTaskHandle);
    }
    else
    {
        ESP_LOGE(k_logTag, "LED task not initialized; cannot send status");
    }
}

void StatusLed::runLedTask(void *p_param)
{
    auto *p_self = static_cast<StatusLed *>(p_param);

    ControlBoardWorkingStatus receivedStatus;
    ESP_LOGI(k_logTag, "LED task started on pin %d", p_self->m_pin);
    while (!p_self->m_stopLedTask.load(std::memory_order_acquire))
    {
        if (xQueueReceive(p_self->mp_statusQueue, &receivedStatus, pdMS_TO_TICKS(100)))
        {
            if (p_self->m_stopLedTask.load(std::memory_order_acquire))
            {
                break;
            }

            p_self->m_currentStatus.store(receivedStatus);
            ESP_LOGI(k_logTag, "LED status updated to %d", static_cast<int>(receivedStatus));
            ESP_LOGI(k_logTag, "Handling status change for pin %d", p_self->m_pin);
            xTimerStop(p_self->mp_blinkTimer, 0);
            p_self->m_ledOn = false;
            p_self->updateDuty(0);

            switch (receivedStatus)
            {
            case ControlBoardWorkingStatus::doingWork:
                p_self->m_ledOn = true;
                p_self->updateDuty(MAX_DUTY);
                break;
            case ControlBoardWorkingStatus::Active:
                // Blip: brief flash once per second (off 900ms, on 100ms)
                p_self->updateDuty(ACT_DUTY);
                xTimerChangePeriod(p_self->mp_blinkTimer, pdMS_TO_TICKS(BLIP_OFF_MS), 0);
                xTimerStart(p_self->mp_blinkTimer, 0);
                break;
            case ControlBoardWorkingStatus::SolidIdle:
                p_self->m_ledOn = true;
                p_self->updateDuty(p_self->m_idleDuty.load());
                break;
            case ControlBoardWorkingStatus::Idle:
                // LED stays off — system is idle/off
                break;
            case ControlBoardWorkingStatus::sleeping:
                p_self->updateDuty(MAX_DUTY);
                xTimerChangePeriod(p_self->mp_blinkTimer, pdMS_TO_TICKS(SLEEP_BLIP_OFF_MS), 0);
                xTimerStart(p_self->mp_blinkTimer, 0);
                break;
            }
        }
    }

    p_self->mp_ledTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void StatusLed::updateDuty(uint32_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, m_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, m_channel);
}

void StatusLed::handleTimer(TimerHandle_t timerHandle)
{
    auto *p_self = static_cast<StatusLed *>(pvTimerGetTimerID(timerHandle));
    p_self->m_ledOn = !p_self->m_ledOn;
    uint32_t duty = p_self->m_ledOn
                        ? p_self->getBlinkDuty(p_self->m_currentStatus.load())
                        : 0;
    p_self->updateDuty(duty);

    if (p_self->m_currentStatus.load() == ControlBoardWorkingStatus::Active)
    {
        const uint32_t nextPeriod = p_self->m_ledOn ? BLIP_ON_MS : BLIP_OFF_MS;
        xTimerChangePeriod(timerHandle, pdMS_TO_TICKS(nextPeriod), 0);
    }
    else if (p_self->m_currentStatus.load() == ControlBoardWorkingStatus::sleeping)
    {
        const uint32_t nextPeriod = p_self->m_ledOn ? BLIP_ON_MS : SLEEP_BLIP_OFF_MS;
        xTimerChangePeriod(timerHandle, pdMS_TO_TICKS(nextPeriod), 0);
    }
}

uint32_t StatusLed::getBlinkDuty(ControlBoardWorkingStatus status)
{
    switch (status)
    {
    case ControlBoardWorkingStatus::SolidIdle:
    case ControlBoardWorkingStatus::Idle:
        return m_idleDuty.load();
    case ControlBoardWorkingStatus::Active:
    case ControlBoardWorkingStatus::sleeping:
        return MAX_DUTY;
    default:
        return MAX_DUTY;
    }
}

void StatusLed::setIdleDuty(uint32_t duty)
{
    m_idleDuty.store(duty, std::memory_order_relaxed);
    // If currently showing SolidIdle, re-send so the task applies the new duty immediately.
    if (m_currentStatus.load(std::memory_order_relaxed) == ControlBoardWorkingStatus::SolidIdle)
        sendStatus(ControlBoardWorkingStatus::SolidIdle);
}