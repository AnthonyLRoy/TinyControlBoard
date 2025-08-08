#include "powerLed.hpp"
#include <algorithm>
#include "esp_log.h"

namespace indicators
{
    PowerLed::PowerLed(gpio_num_t PIN_APP_ACTIVE_LED, gpio_num_t PIN_APP_STANDBY_LEDl, ledc_channel_t channel)
    {

        dutyCycle = 4096;
        ESP_LOGI("PowerLed", "Initializing PowerLed with channel: %d", channel);
        ledc_timer_config_t ledc_timer = {};
        ledc_timer.speed_mode = LEDC_MODE;
        ledc_timer.duty_resolution = LEDC_DUTY_RES;
        ledc_timer.timer_num = LEDC_TIMER_1;
        ledc_timer.freq_hz = LEDC_FREQUENCY; // Set output frequency at 4 kHz
        ledc_timer.clk_cfg = LEDC_AUTO_CLK;

        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel_on = {};
        ledc_channel_on.channel    = LEDC_CHANNEL_1;
        ledc_channel_on.duty       = 0;
        ledc_channel_on.gpio_num   = PIN_APP_ACTIVE_LED;
        ledc_channel_on.speed_mode = LEDC_MODE;
        ledc_channel_on.hpoint     = 0;
        ledc_channel_on.timer_sel  = LEDC_TIMER;

        ledc_channel_config(&ledc_channel_on);

        ledc_channel_config_t ledc_channel_standBy = {};
        ledc_channel_standBy.channel    = LEDC_CHANNEL_2;
        ledc_channel_standBy.duty       = 0;
        ledc_channel_standBy.gpio_num   = PIN_APP_STANDBY_LEDl;
        ledc_channel_standBy.speed_mode = LEDC_MODE;
        ledc_channel_standBy.hpoint     = 0;
        ledc_channel_standBy.timer_sel  = LEDC_TIMER;

        ledc_channel_config(&ledc_channel_standBy);

        onLed.init(ledc_timer, ledc_channel_on);

        standBy.init(ledc_timer, ledc_channel_standBy); // Initialize the LED channels
    };
    PowerLed::~PowerLed() {
    };

    void PowerLed::setState(ControlBoardState state)
    {
        switch (state)
        {
        case ControlBoardState::Active:
            onLed.setDuty(dutyCycle);
            onLed.updateDuty();
            standBy.setDuty(LED_OFF); // set to 0
            standBy.updateDuty();
            break;
        case ControlBoardState::Standby:
        ESP_LOGI("PowerLed", "Setting state to Standby with duty cycle: %d", dutyCycle);

            standBy.setDuty(dutyCycle); // 50% duty cycle
            standBy.updateDuty();
            onLed.setDuty(LED_OFF); // 0% duty cycle
            onLed.updateDuty();
            break;
        default:
            ESP_LOGI("PowerLed", "Unknown state: %d, turning off all LEDs", static_cast<int>(state));
            onLed.setDuty(LED_OFF);   // 0% duty cycle
            standBy.setDuty(LED_OFF); // 0% duty cycle
            onLed.updateDuty();
            standBy.updateDuty();
            ESP_LOGW("PowerLed", "Unknown state: %d, turning off all LEDs", static_cast<int>(state));
            break;
        };
    };

    void PowerLed::setBrightness(int brightness)
    {
        brightness = std::clamp(brightness, 0, 99);
        dutyCycle = (brightness * 4096) / 100; // Convert percentage to duty cycle for 13-bit resolution
    }

    void PowerLed::update() {
        // This function can be used to update the LED state if needed
        // For example, you might want to blink the LED or change its brightness
        // based on some conditions.
    };
};