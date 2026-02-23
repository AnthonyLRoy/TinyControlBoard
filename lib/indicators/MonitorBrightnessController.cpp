#include "monitorBrightnessController.hpp"
#include <cmath>

#define TAG "MonitorBrightnessController"

namespace indicators
{
    MonitorBrightnessController::~MonitorBrightnessController()
    {
    }

    void MonitorBrightnessController::init()
    {
        ESP_LOGI(TAG, "Initializing MonitorBrightnessController hardware");

        // LEDC timer
        ledc_timer_config_t timer = {};
        timer.speed_mode = LEDC_MODE;
        timer.duty_resolution = LEDC_DUTY_RES;
        timer.timer_num = LEDC_TIMER_3;
        timer.freq_hz = 4000;
        timer.clk_cfg = LEDC_AUTO_CLK;
        ledc_timer_config(&timer);

        // Active LED
        ledc_channel_config_t activeCfg = {};
        activeCfg.channel = pwmChannel;
        activeCfg.duty = 0;
        activeCfg.gpio_num = monitorPin;
        activeCfg.speed_mode = LEDC_MODE;
        activeCfg.hpoint = 0;
        activeCfg.timer_sel = LEDC_TIMER_3;
        ledc_channel_config(&activeCfg);

        // Init PWM wrappers
        monitorLed.init(timer, activeCfg);

        started = true;
    }

    void MonitorBrightnessController::ChangeBrightnessLevel(int change)
    {
        currentBrightnessLevel += change;
        if (currentBrightnessLevel < 0)
            currentBrightnessLevel = 0;
        else if (currentBrightnessLevel > 9)
            currentBrightnessLevel = 9;
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
