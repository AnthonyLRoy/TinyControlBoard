#include "powerLed.hpp"
#include <cmath>

static const char *spTag = "Power_Led       ";

namespace indicators
{

PowerLed::PowerLed(gpio_num_t aPin, ledc_channel_t aChannel,
                                     gpio_num_t sPin, ledc_channel_t sChannel)
        : mActivePin(aPin), mActiveChannel(aChannel),
            mStandbyPin(sPin), mStandbyChannel(sChannel)
{
    // Just store values; do not create timers here
}


PowerLed::~PowerLed()
{
    if (mpUpdateTimer)
    {
        esp_timer_stop(mpUpdateTimer);
        esp_timer_delete(mpUpdateTimer);
    }
}

void PowerLed::init()
{
    ESP_LOGI(spTag, "Initializing PowerLed hardware");

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
    activeCfg.channel = mActiveChannel;
    activeCfg.duty = 0;
    activeCfg.gpio_num = mActivePin;
    activeCfg.speed_mode = LEDC_MODE;
    activeCfg.hpoint = 0;
    activeCfg.timer_sel = LEDC_TIMER;
    ledc_channel_config(&activeCfg);

    // Standby LED
    ledc_channel_config_t standbyCfg = {};
    standbyCfg.channel = mStandbyChannel;
    standbyCfg.duty = 0;
    standbyCfg.gpio_num = mStandbyPin;
    standbyCfg.speed_mode = LEDC_MODE;
    standbyCfg.hpoint = 0;
    standbyCfg.timer_sel = LEDC_TIMER;
    ledc_channel_config(&standbyCfg);

    // Init PWM wrappers
    mActiveLed.init(timer, activeCfg);
    mStandbyLed.init(timer, standbyCfg);

    // Create periodic update timer
    esp_timer_create_args_t args = {};
    args.callback = &PowerLed::handleTimer;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "power_led_update";

    ESP_ERROR_CHECK(esp_timer_create(&args, &mpUpdateTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(mpUpdateTimer, 50 * 1000)); // 50 ms

  
}


void PowerLed::handleTimer(void *pArg)
{
    PowerLed *pSelf = static_cast<PowerLed *>(pArg);
    pSelf->update();
}


void PowerLed::setBrightness(int brightness)
{
    if (brightness < 0) brightness = 0;
    else if (brightness > 99) brightness = 99;

    mDutyCycle = (brightness * 4096) / 100;
}


void PowerLed::setState(ControlBoardPowerState state)
{
    mCurrentPowerState = state;
    mActiveFlash = false;
    mStandbyFlash = false;
    mActiveBreathing = false;
    mStandbyBreathing = false;
    mActiveBlip = false;
    mStandbyBlip = false;
    uint64_t now = esp_timer_get_time() / 1000;
    ESP_LOGI(spTag, "Setting PowerLed state to %d", static_cast<int>(state));

    switch (state)
    {
        case ControlBoardPowerState::OFF:
            mActiveLed.setDuty(mOffDuty);
            mStandbyLed.setDuty(mMediumDuty);
            break;

        case ControlBoardPowerState::SHUTTING_DOWN:
            mActiveFlash = true;
            mStandbyLed.setDuty(mOffDuty);
            break;

        case ControlBoardPowerState::ON:
            mActiveLed.setDuty(mDutyCycle);
            mStandbyLed.setDuty(mOffDuty);
            break;

        case ControlBoardPowerState::TURNING_ON:
            mStandbyFlash = true;
            mActiveLed.setDuty(mOffDuty);
            break;

        case ControlBoardPowerState::SLEEP:
            mActiveLed.setDuty(mOffDuty);
            mStandbyBreathing = true;
            mBreathingStartTime = now;
            break;

        case ControlBoardPowerState::GOING_TO_SLEEP:
            mActiveLed.setDuty(mOffDuty);
            mStandbyFlash = true;
            break;

        case ControlBoardPowerState::DEEPSLEEP:
            mActiveLed.setDuty(mOffDuty);
            mStandbyBlip = true;
            mBlipStartTime = now;
            break;

        case ControlBoardPowerState::GOING_INTO_DEEP_SLEEP:
            mActiveLed.setDuty(mOffDuty);
            mStandbyFlash = true;
            break;

        default:
                mActiveLed.setDuty(mOffDuty);
                mStandbyLed.setDuty(mOffDuty);
            break;
    }

            mActiveLed.updateDuty();
            mStandbyLed.updateDuty();
}


void PowerLed::update()
{
    const uint32_t flashPeriod = 300; // ms
    uint64_t now = esp_timer_get_time() / 1000;

    // Handle regular flashing
    if ((mActiveFlash || mStandbyFlash) && (now - mLastFlashToggle >= flashPeriod))
    {
        mLastFlashToggle = now;
        mFlashState = !mFlashState;

        if (mActiveFlash)
        {
            mActiveLed.setDuty(mFlashState ? mDutyCycle : mOffDuty);
            mActiveLed.updateDuty();
        }

        if (mStandbyFlash)
        {
            mStandbyLed.setDuty(mFlashState ? mDutyCycle : mOffDuty);
            mStandbyLed.updateDuty();
        }
    }

    // Handle breathing effect (smooth fade in/out)
    if (mActiveBreathing || mStandbyBreathing)
    {
        uint64_t elapsed = now - mBreathingStartTime;
        uint32_t phase = elapsed % BREATHING_PERIOD;
        
        // Use sine-like breathing: 0->max->0 over the period
        // phase goes from 0 to BREATHING_PERIOD
        float ratio = (float)phase / BREATHING_PERIOD;
        // Create smooth breathing curve (sine wave from 0 to 1 to 0)
        float sineValue = sinf(ratio * 3.14159f); // 0 to pi gives 0->1->0
        int breathingDuty = (int)(2048 * sineValue); // 50% brightness

        if (mActiveBreathing)
        {
            mActiveLed.setDuty(breathingDuty);
            mActiveLed.updateDuty();
        }

        if (mStandbyBreathing)
        {
            mStandbyLed.setDuty(breathingDuty);
            mStandbyLed.updateDuty();
        }
    }

    // Handle blip effect (short pulse every 10 seconds)
    if (mActiveBlip || mStandbyBlip)
    {
        uint64_t elapsed = now - mBlipStartTime;
        uint32_t cyclePhase = elapsed % BLIP_PERIOD;
        bool shouldBeOn = (cyclePhase < BLIP_DURATION);
        
        int blipDuty = shouldBeOn ? 2048 : mOffDuty; // 50% brightness

        if (mActiveBlip)
        {
            mActiveLed.setDuty(blipDuty);
            mActiveLed.updateDuty();
        }

        if (mStandbyBlip)
        {
            mStandbyLed.setDuty(blipDuty);
            mStandbyLed.updateDuty();
        }
    }
}

} // namespace indicators
