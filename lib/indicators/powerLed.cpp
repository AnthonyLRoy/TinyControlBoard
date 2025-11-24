#include "powerLed.hpp"

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

    // Active LED
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
            activeFlash = true;
            standbyLed.setDuty(offDuty);
            break;

        case ControlBoardPowerState::SLEEP:
            activeLed.setDuty(offDuty);
            standbyLed.setDuty(mediumDuty);
            break;

        case ControlBoardPowerState::GOING_TO_SLEEP:
            activeLed.setDuty(offDuty);
            standbyFlash = true;
            break;

        case ControlBoardPowerState::DEEPSLEEP:
            activeLed.setDuty(offDuty);
            standbyLed.setDuty(mediumDuty);
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

    if (!(activeFlash || standbyFlash)) return;

    if (now - lastFlashToggle >= flashPeriod)
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
}

} // namespace indicators
