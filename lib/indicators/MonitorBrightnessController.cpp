#include "monitorBrightnessController.hpp"
#include <cmath>
#include <algorithm>

static const char *spTag = "MonitorBrightnessController";

namespace indicators
{
    MonitorBrightnessController::~MonitorBrightnessController()
    {
    }

    void MonitorBrightnessController::init()
    {
        ESP_LOGI(spTag, "Initializing MonitorBrightnessController hardware");
        ESP_LOGI(spTag, "monitorPin: %d, pwmChannel: %d", mMonitorPin, mPwmChannel);

        // LEDC timer
        ledc_timer_config_t timer = {};
        timer.speed_mode = LEDC_MODE;
        timer.duty_resolution = LEDC_DUTY_RES;
        timer.timer_num = LEDC_TIMER_0;
        timer.freq_hz = 4000;
        timer.clk_cfg = LEDC_AUTO_CLK;
        if (ledc_timer_config(&timer) != ESP_OK) {
            ESP_LOGE(spTag, "Failed to configure LEDC timer");
        }

        // Active LED
        ledc_channel_config_t activeCfg = {};
        activeCfg.channel = mPwmChannel;
        activeCfg.duty = 0;
        activeCfg.gpio_num = mMonitorPin;
        activeCfg.speed_mode = LEDC_MODE;
        activeCfg.hpoint = 0;
        activeCfg.timer_sel = LEDC_TIMER_0;
        if (ledc_channel_config(&activeCfg) != ESP_OK) {
            ESP_LOGE(spTag, "Failed to configure LEDC channel");
        }

        // Init PWM wrappers
        mMonitorLed.init(timer, activeCfg);

        mStarted = true;
    }

    void MonitorBrightnessController::changeBrightnessLevel(int change)
    {
        ESP_LOGI(spTag, "Changing brightness level by %d", change);
        mCurrentBrightnessLevel = std::clamp(mCurrentBrightnessLevel, 0, 9);
        mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
        mMonitorLed.updateDuty();

    }

    void MonitorBrightnessController::cycleBrightness()
    {
        mCurrentBrightnessLevel = (mCurrentBrightnessLevel + 1) % 10;
        mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
        mMonitorLed.updateDuty();
    }


} // namespace indicators
