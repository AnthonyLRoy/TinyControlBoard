#include "monitorBrightnessController.hpp"
#include <cmath>
#include <algorithm>

static const char *spTag = "Monitor_Bright  ";

namespace indicators
{
    namespace
    {
        uint32_t getDutyForBrightnessLevel(int brightnessLevel)
        {
            const int clampedLevel = std::clamp(brightnessLevel, 0, 9);
            constexpr uint32_t kBrightnessDuties[10] = {0, 500, 750, 1000, 1250, 1500, 2000, 3000, 3500, 4000};
            return kBrightnessDuties[clampedLevel];
        }
    }

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
        mCurrentBrightnessLevel = std::clamp(mCurrentBrightnessLevel + change, 0, 9);
        mBlanked = false;
        mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
        mMonitorLed.updateDuty();

    }

    void MonitorBrightnessController::cycleBrightness()
    {
        mCurrentBrightnessLevel = (mCurrentBrightnessLevel + 1) % 10;
        mBlanked = false;
        mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
        mMonitorLed.updateDuty();
    }

    void MonitorBrightnessController::setBlanked(bool blanked)
    {
        mBlanked = blanked;
        const uint32_t duty = blanked ? 0 : getDutyForBrightnessLevel(mCurrentBrightnessLevel);
        ESP_LOGI(spTag, "%s monitor backlight", blanked ? "Blanking" : "Restoring");
        mMonitorLed.setDuty(duty);
        mMonitorLed.updateDuty();
    }


} // namespace indicators
