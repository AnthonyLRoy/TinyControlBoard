#include "powerLed.hpp"
#include <algorithm>
#include "esp_log.h"
#include "esp_timer.h"


#define TAG "PowerLed"
namespace indicators
{
    PowerLed::PowerLed(gpio_num_t PIN_APP_ACTIVE_LED,ledc_channel_t onChannel, gpio_num_t PIN_APP_STANDBY_LEDl, ledc_channel_t offChannel)
    {

        dutyCycle = 4096;
        ESP_LOGI(TAG, "Initializing PowerLed with channel: %d  off Channel %d", onChannel,offChannel);
        ledc_timer_config_t ledc_timer = {};
        ledc_timer.speed_mode = LEDC_MODE;
        ledc_timer.duty_resolution = LEDC_DUTY_RES;
        ledc_timer.timer_num = LEDC_TIMER_1;
        ledc_timer.freq_hz = LEDC_FREQUENCY; // Set output frequency at 4 kHz
        ledc_timer.clk_cfg = LEDC_AUTO_CLK;

        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel_on = {};
        ledc_channel_on.channel    = onChannel;
        ledc_channel_on.duty       = 0;
        ledc_channel_on.gpio_num   = PIN_APP_ACTIVE_LED;
        ledc_channel_on.speed_mode = LEDC_MODE;
        ledc_channel_on.hpoint     = 0;
        ledc_channel_on.timer_sel  = LEDC_TIMER;

        ledc_channel_config(&ledc_channel_on);

        ledc_channel_config_t ledc_channel_standBy = {};
        ledc_channel_standBy.channel    = offChannel;
        ledc_channel_standBy.duty       = 0;
        ledc_channel_standBy.gpio_num   = PIN_APP_STANDBY_LEDl;
        ledc_channel_standBy.speed_mode = LEDC_MODE;
        ledc_channel_standBy.hpoint     = 0;
        ledc_channel_standBy.timer_sel  = LEDC_TIMER;

        ledc_channel_config(&ledc_channel_standBy);

        onLed.init(ledc_timer, ledc_channel_on);

        standByLed.init(ledc_timer, ledc_channel_standBy); // Initialize the LED channels
    };
    PowerLed::~PowerLed() {
    };

    void PowerLed::setState(ControlBoardPowerState state)
    {
        currentPowerState = state;
        uint64_t currentTime = esp_timer_get_time() / 1000; // Current time in ms
        ESP_LOGI(TAG, "PowerLed::setState called with state: %d", static_cast<int>(state));

        switch (state)
        {
        case ControlBoardPowerState::ON:
            ESP_LOGI(TAG, "Setting state to ON");
            onLed.setDuty(dutyCycle);
            onLed.updateDuty();
            standByLed.setDuty(LED_OFF);
            standByLed.updateDuty();
   
            break;
            
        case ControlBoardPowerState::SLEEP:
            ESP_LOGI(TAG, "Setting state to SLEEP");
            onLed.setDuty(LED_OFF);
            onLed.updateDuty();
            standByLed.setDuty(mediumDutyCycle);
            standByLed.updateDuty();
            break;
            
        case ControlBoardPowerState::SHUTTING_DOWN:
            ESP_LOGI(TAG, "Setting state to SHUTTING_DOWN");
            onLed.setDuty(mediumDutyCycle);
            onLed.updateDuty();
            standByLed.setDuty(LED_OFF);
            standByLed.updateDuty();
            break;
            
        case ControlBoardPowerState::TURNING_ON:
            ESP_LOGI(TAG, "Setting state to TURNING_ON");
            onLed.setDuty(LED_OFF);
            onLed.updateDuty();
            standByLed.setDuty(mediumDutyCycle);
            standByLed.updateDuty();
            break;
            
        case ControlBoardPowerState::GOING_TO_SLEEP:
            ESP_LOGI(TAG, "Setting state to GOING_TO_SLEEP");
            onLed.setDuty(mediumDutyCycle);
            onLed.updateDuty();
            standByLed.setDuty(LED_OFF);
            standByLed.updateDuty();
            break;
            
        case ControlBoardPowerState::DEEPSLEEP:
            ESP_LOGI(TAG, "Setting state to DEEPSLEEP");
            onLed.setDuty(LED_OFF);
            onLed.updateDuty();
            standByLed.setDuty(mediumDutyCycle);
            standByLed.updateDuty();
            break;
            
        case ControlBoardPowerState::GOING_INTO_DEEP_SLEEP:
            ESP_LOGI(TAG, "Setting state to GOING_INTO_DEEP_SLEEP");
            onLed.setDuty(LED_OFF);
            onLed.updateDuty();
            standByLed.setDuty(mediumDutyCycle);
            standByLed.updateDuty();
            break;
            
        case ControlBoardPowerState::OFF:
        default:
            ESP_LOGI(TAG, "Setting state to OFF");
            onLed.setDuty(LED_OFF);
            standByLed.setDuty(LED_OFF);
            onLed.updateDuty();
            standByLed.updateDuty();
            break;
        };
    };

    ControlBoardPowerState PowerLed::getState() const
    {
        return currentPowerState;
    }

    void PowerLed::setBrightness(int brightness)
    {
        brightness = std::clamp(brightness, 0, 99);
        dutyCycle = (brightness * 4096) / 100; // Convert percentage to duty cycle for 13-bit resolution
    }

    
};