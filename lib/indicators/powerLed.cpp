#include "powerLed.hpp"
#include <cmath>

static constexpr const char *k_logTag = "Power_Led       ";

namespace indicators
{

PowerLed::PowerLed(gpio_num_t aPin, ledc_channel_t aChannel,
                                     gpio_num_t s_pin, ledc_channel_t s_channel)
        : m_activePin(aPin), m_activeChannel(aChannel),
            m_standbyPin(s_pin), m_standbyChannel(s_channel)
{
    // Just store values; do not create timers here it won't work because the ESP32 timer system is not initialized yet.
}


PowerLed::~PowerLed()
{
    if (mp_updateTimer)
    {
        esp_timer_stop(mp_updateTimer);
        esp_timer_delete(mp_updateTimer);
    }
}

void PowerLed::init()
{
    ESP_LOGI(k_logTag, "Initializing PowerLed hardware");
    m_started = false;

    // LEDC timer
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_MODE;
    timer.duty_resolution = LEDC_DUTY_RES;
    timer.timer_num = LEDC_TIMER_1;
    timer.freq_hz = LEDC_FREQUENCY;
    timer.clk_cfg = LEDC_AUTO_CLK;
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to configure LEDC timer (err=0x%x)", err);
        return;
    }

    // Monitor  LED
    ledc_channel_config_t activeCfg = {};
    activeCfg.channel = m_activeChannel;
    activeCfg.duty = 0;
    activeCfg.gpio_num = m_activePin;
    activeCfg.speed_mode = LEDC_MODE;
    activeCfg.hpoint = 0;
    activeCfg.timer_sel = LEDC_TIMER;
    err = ledc_channel_config(&activeCfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to configure active LED channel (err=0x%x)", err);
        return;
    }

    // Standby LED
    ledc_channel_config_t standbyCfg = {};
    standbyCfg.channel = m_standbyChannel;
    standbyCfg.duty = 0;
    standbyCfg.gpio_num = m_standbyPin;
    standbyCfg.speed_mode = LEDC_MODE;
    standbyCfg.hpoint = 0;
    standbyCfg.timer_sel = LEDC_TIMER;
    err = ledc_channel_config(&standbyCfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to configure standby LED channel (err=0x%x)", err);
        return;
    }

    // Init PWM wrappers
    m_activeLed.init(timer, activeCfg);
    m_standbyLed.init(timer, standbyCfg);

    // Create periodic update timer
    esp_timer_create_args_t args = {};
    args.callback = &PowerLed::handleTimer;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "power_led_update";

    err = esp_timer_create(&args, &mp_updateTimer);
    if (err != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to create PowerLed timer (err=0x%x)", err);
        return;
    }

    err = esp_timer_start_periodic(mp_updateTimer, 50 * 1000);
    if (err != ESP_OK)
    {
        ESP_LOGE(k_logTag, "Failed to start PowerLed timer (err=0x%x)", err);
        return;
    }

    m_started = true;

  
}


void PowerLed::handleTimer(void *p_arg)
{
    PowerLed *p_self = static_cast<PowerLed *>(p_arg);
    p_self->update();
}


void PowerLed::setBrightness(int brightness)
{
    if (brightness < 0) brightness = 0;
    else if (brightness > 99) brightness = 99;

    m_dutyCycle = (brightness * 4096) / 100;
}


void PowerLed::setState(ControlBoardPowerState state)
{
    if (!m_started)
    {
        ESP_LOGW(k_logTag, "Ignoring setState because PowerLed is not initialized");
        return;
    }

    m_currentPowerState = state;
    m_activeFlash = false;
    m_standbyFlash = false;
    m_activeBreathing = false;
    m_standbyBreathing = false;
    m_activeBlip = false;
    m_standbyBlip = false;
    uint64_t now = esp_timer_get_time() / 1000;
    ESP_LOGI(k_logTag, "Setting PowerLed state to %d", static_cast<int>(state));

    switch (state)
    {
        case ControlBoardPowerState::OFF:
            m_activeLed.setDuty(m_offDuty);
            m_standbyLed.setDuty(m_mediumDuty);
            break;

        case ControlBoardPowerState::SHUTTING_DOWN:
            m_activeFlash = true;
            m_standbyLed.setDuty(m_offDuty);
            break;

        case ControlBoardPowerState::ON:
            m_activeLed.setDuty(m_dutyCycle);
            m_standbyLed.setDuty(m_offDuty);
            break;

        case ControlBoardPowerState::TURNING_ON:
            m_standbyFlash = true;
            m_activeLed.setDuty(m_offDuty);
            break;

        case ControlBoardPowerState::SLEEP:
            m_activeLed.setDuty(m_offDuty);
            m_standbyBreathing = true;
            m_breathingStartTime = now;
            break;

        case ControlBoardPowerState::GOING_TO_SLEEP:
            m_activeLed.setDuty(m_offDuty);
            m_standbyFlash = true;
            break;

        case ControlBoardPowerState::DEEPSLEEP:
            m_activeLed.setDuty(m_offDuty);
            m_standbyBlip = true;
            m_blipStartTime = now;
            break;

        case ControlBoardPowerState::GOING_INTO_DEEP_SLEEP:
            m_activeLed.setDuty(m_offDuty);
            m_standbyFlash = true;
            break;

        default:
                m_activeLed.setDuty(m_offDuty);
                m_standbyLed.setDuty(m_offDuty);
            break;
    }

            m_activeLed.updateDuty();
            m_standbyLed.updateDuty();
}


void PowerLed::update()
{
    if (!m_started)
    {
        return;
    }

    const uint32_t flashPeriod = 300; // ms
    uint64_t now = esp_timer_get_time() / 1000;

    // Handle regular flashing
    if ((m_activeFlash || m_standbyFlash) && (now - m_lastFlashToggle >= flashPeriod))
    {
        m_lastFlashToggle = now;
        m_flashState = !m_flashState;

        if (m_activeFlash)
        {
            m_activeLed.setDuty(m_flashState ? m_dutyCycle : m_offDuty);
            m_activeLed.updateDuty();
        }

        if (m_standbyFlash)
        {
            m_standbyLed.setDuty(m_flashState ? m_dutyCycle : m_offDuty);
            m_standbyLed.updateDuty();
        }
    }

    // Handle breathing effect (smooth fade in/out)
    if (m_activeBreathing || m_standbyBreathing)
    {
        uint64_t elapsed = now - m_breathingStartTime;
        uint32_t phase = elapsed % BREATHING_PERIOD;
        
        // create breathing: 0->max->0 over the period
        // phase goes from 0 to BREATHING_PERIOD
        float ratio = (float)phase / BREATHING_PERIOD;
        // Create sine wave
        float sineValue = sinf(ratio * 3.14159f); // 0 to pi gives 0->1->0
        int breathingDuty = (int)(2048 * sineValue); // 50% brightness

        if (m_activeBreathing)
        {
            m_activeLed.setDuty(breathingDuty);
            m_activeLed.updateDuty();
        }

        if (m_standbyBreathing)
        {
            m_standbyLed.setDuty(breathingDuty);
            m_standbyLed.updateDuty();
        }
    }

    //  blip   pulse every 10 seconds)
    if (m_activeBlip || m_standbyBlip)
    {
        uint64_t elapsed = now - m_blipStartTime;
        uint32_t cyclePhase = elapsed % BLIP_PERIOD;
        bool shouldBeOn = (cyclePhase < BLIP_DURATION);
        
        int blipDuty = shouldBeOn ? 2048 : m_offDuty; // 50% brightness

        if (m_activeBlip)
        {
            m_activeLed.setDuty(blipDuty);
            m_activeLed.updateDuty();
        }

        if (m_standbyBlip)
        {
            m_standbyLed.setDuty(blipDuty);
            m_standbyLed.updateDuty();
        }
    }
}

} // namespace indicators
