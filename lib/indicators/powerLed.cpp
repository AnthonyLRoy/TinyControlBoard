#include "powerLed.hpp"
#include <cmath>

#define TAG "PowerLed"

namespace indicators
{

PowerLed::PowerLed(gpio_num_t aPin, ledc_channel_t aChannel,
                   gpio_num_t sPin, ledc_channel_t sChannel)
    : activePin(aPin), activeChannel(aChannel),
      standbyPin(sPin), standbyChannel(sChannel)
{
    // Just store values; do not create timers here
}


PowerLed::~PowerLed()
{
    if (updateTimer)
    {
        esp_timer_stop(updateTimer);
        esp_timer_delete(updateTimer);
    }
}

void PowerLed::init()
{
    ESP_LOGI(TAG, "Initializing PowerLed hardware");

    // LEDC timer
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_MODE;
    timer.duty_resolution = LEDC_DUTY_RES;
    timer.timer_num = LEDC_TIMER_1;
    timer.freq_hz = LEDC_FREQUENCY;
    timer.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer);

    // Monitor  LED
    ledc_channel_config_t activeCfg = {};
    activeCfg.channel = activeChannel;
    activeCfg.duty = 0;
    activeCfg.gpio_num = activePin;
    activeCfg.speed_mode = LEDC_MODE;
    activeCfg.hpoint = 0;
    activeCfg.timer_sel = LEDC_TIMER;
    ledc_channel_config(&activeCfg);

    // Standby LED
    ledc_channel_config_t standbyCfg = {};
    standbyCfg.channel = standbyChannel;
    standbyCfg.duty = 0;
    standbyCfg.gpio_num = standbyPin;
    standbyCfg.speed_mode = LEDC_MODE;
    standbyCfg.hpoint = 0;
    standbyCfg.timer_sel = LEDC_TIMER;
    ledc_channel_config(&standbyCfg);

    // Init PWM wrappers
    activeLed.init(timer, activeCfg);
    standbyLed.init(timer, standbyCfg);

    // Create periodic update timer
    esp_timer_create_args_t args = {};
    args.callback = &PowerLed::timerCallback;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "power_led_update";

    ESP_ERROR_CHECK(esp_timer_create(&args, &updateTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(updateTimer, 50 * 1000)); // 50 ms

  
}


void PowerLed::timerCallback(void *arg)
{
    PowerLed *self = static_cast<PowerLed *>(arg);
    self->update();
}


void PowerLed::setBrightness(int brightness)
{
    if (brightness < 0) brightness = 0;
    else if (brightness > 99) brightness = 99;

    dutyCycle = (brightness * 4096) / 100;
}


void PowerLed::setState(ControlBoardPowerState state)
{
    currentPowerState = state;
    activeFlash = false;
    standbyFlash = false;
    activeBreathing = false;
    standbyBreathing = false;
    activeBlip = false;
    standbyBlip = false;
    uint64_t now = esp_timer_get_time() / 1000;
    ESP_LOGI(TAG, "Setting PowerLed state to %d", static_cast<int>(state));

    switch (state)
    {
        case ControlBoardPowerState::OFF:
            activeLed.setDuty(offDuty);
            standbyLed.setDuty(mediumDuty);
            break;

        case ControlBoardPowerState::SHUTTING_DOWN:
            activeFlash = true;
            standbyLed.setDuty(offDuty);
            break;

        case ControlBoardPowerState::ON:
            activeLed.setDuty(dutyCycle);
            standbyLed.setDuty(offDuty);
            break;

        case ControlBoardPowerState::TURNING_ON:
            standbyFlash = true;
            activeLed.setDuty(offDuty);
            break;

        case ControlBoardPowerState::SLEEP:
            activeLed.setDuty(offDuty);
            standbyBreathing = true;
            breathingStartTime = now;
            break;

        case ControlBoardPowerState::GOING_TO_SLEEP:
            activeLed.setDuty(offDuty);
            standbyFlash = true;
            break;

        case ControlBoardPowerState::DEEPSLEEP:
            activeLed.setDuty(offDuty);
            standbyBlip = true;
            blipStartTime = now;
            break;

        case ControlBoardPowerState::GOING_INTO_DEEP_SLEEP:
            activeLed.setDuty(offDuty);
            standbyFlash = true;
            break;

        default:
            activeLed.setDuty(offDuty);
            standbyLed.setDuty(offDuty);
            break;
    }

    activeLed.updateDuty();
    standbyLed.updateDuty();
}


void PowerLed::update()
{
    const uint32_t flashPeriod = 300; // ms
    uint64_t now = esp_timer_get_time() / 1000;

    // Handle regular flashing
    if ((activeFlash || standbyFlash) && (now - lastFlashToggle >= flashPeriod))
    {
        lastFlashToggle = now;
        flashState = !flashState;

        if (activeFlash)
        {
            activeLed.setDuty(flashState ? dutyCycle : offDuty);
            activeLed.updateDuty();
        }

        if (standbyFlash)
        {
            standbyLed.setDuty(flashState ? dutyCycle : offDuty);
            standbyLed.updateDuty();
        }
    }

    // Handle breathing effect (smooth fade in/out)
    if (activeBreathing || standbyBreathing)
    {
        uint64_t elapsed = now - breathingStartTime;
        uint32_t phase = elapsed % BREATHING_PERIOD;
        
        // Use sine-like breathing: 0->max->0 over the period
        // phase goes from 0 to BREATHING_PERIOD
        float ratio = (float)phase / BREATHING_PERIOD;
        // Create smooth breathing curve (sine wave from 0 to 1 to 0)
        float sineValue = sinf(ratio * 3.14159f); // 0 to pi gives 0->1->0
        int breathingDuty = (int)(2048 * sineValue); // 50% brightness

        if (activeBreathing)
        {
            activeLed.setDuty(breathingDuty);
            activeLed.updateDuty();
        }

        if (standbyBreathing)
        {
            standbyLed.setDuty(breathingDuty);
            standbyLed.updateDuty();
        }
    }

    // Handle blip effect (short pulse every 10 seconds)
    if (activeBlip || standbyBlip)
    {
        uint64_t elapsed = now - blipStartTime;
        uint32_t cyclePhase = elapsed % BLIP_PERIOD;
        bool shouldBeOn = (cyclePhase < BLIP_DURATION);
        
        int blipDuty = shouldBeOn ? 2048 : offDuty; // 50% brightness

        if (activeBlip)
        {
            activeLed.setDuty(blipDuty);
            activeLed.updateDuty();
        }

        if (standbyBlip)
        {
            standbyLed.setDuty(blipDuty);
            standbyLed.updateDuty();
        }
    }
}

} // namespace indicators
