#include "monitorBrightnessController.hpp"
#include <cmath>
#include <algorithm>

#define TAG "MonitorBrightnessController"

namespace indicators
{
    MonitorBrightnessController::~MonitorBrightnessController()
    {
    }

    void MonitorBrightnessController::init()
    {
        ESP_LOGI(TAG, "Initializing MonitorBrightnessController hardware");
        ESP_LOGI(TAG, "monitorPin: %d, pwmChannel: %d", monitorPin, pwmChannel);

        // LEDC timer
        ledc_timer_config_t timer = {};
        timer.speed_mode = LEDC_MODE;
        timer.duty_resolution = LEDC_DUTY_RES;
        timer.timer_num = LEDC_TIMER_0;
        timer.freq_hz = 4000;
        timer.clk_cfg = LEDC_AUTO_CLK;
        if (ledc_timer_config(&timer) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure LEDC timer");
        }

        // Active LED
        ledc_channel_config_t activeCfg = {};
        activeCfg.channel = pwmChannel;
        activeCfg.duty = 0;
        activeCfg.gpio_num = monitorPin;
        activeCfg.speed_mode = LEDC_MODE;
        activeCfg.hpoint = 0;
        activeCfg.timer_sel = LEDC_TIMER_0;
        if (ledc_channel_config(&activeCfg) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure LEDC channel");
        }

        // Init PWM wrappers
        monitorLed.init(timer, activeCfg);

        started = true;
    }

    void MonitorBrightnessController::ChangeBrightnessLevel(int change)
    {
        ESP_LOGI(TAG, "Changing brightness level by %d", change);
       currentBrightnessLevel = std::clamp(currentBrightnessLevel, 0, 9);
        monitorLed.setDuty(BrightnessLevels[currentBrightnessLevel]);
        monitorLed.updateDuty();

    }

    void MonitorBrightnessController::cycleBrightness()
    {
        currentBrightnessLevel = (currentBrightnessLevel + 1) % 10;
        monitorLed.setDuty(BrightnessLevels[currentBrightnessLevel]);
        monitorLed.updateDuty();
    }


} // namespace indicators
