#include "monitorBrightnessController.hpp"
#include <cmath>
#include <algorithm>
#include "nvs_flash.h"
#include "nvs.h"

namespace
{
    constexpr const char *kNvsNamespace = "brightness";
    constexpr const char *kNvsKeyLevel  = "level";
}

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

        // Load persisted brightness level from NVS
        nvs_handle_t handle;
        if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) == ESP_OK)
        {
            int8_t savedLevel = static_cast<int8_t>(mCurrentBrightnessLevel);
            esp_err_t err = nvs_get_i8(handle, kNvsKeyLevel, &savedLevel);
            if (err == ESP_OK)
            {
                mCurrentBrightnessLevel = std::clamp(static_cast<int>(savedLevel), 0, 9);
                ESP_LOGI(spTag, "Restored brightness level %d from NVS", mCurrentBrightnessLevel);
            }
            else
            {
                // First boot — write the default so it exists next time
                nvs_set_i8(handle, kNvsKeyLevel, static_cast<int8_t>(mCurrentBrightnessLevel));
                nvs_commit(handle);
                ESP_LOGI(spTag, "NVS: first boot, wrote default brightness %d", mCurrentBrightnessLevel);
            }
            nvs_close(handle);
        }
        mSavedBrightnessLevel = mCurrentBrightnessLevel;

        // Apply the loaded level
        mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
        mMonitorLed.updateDuty();

        mStarted = true;
    }

    void MonitorBrightnessController::changeBrightnessLevel(int change)
    {
        ESP_LOGI(spTag, "Changing brightness level by %d", change);
        mCurrentBrightnessLevel = std::clamp(mCurrentBrightnessLevel + change, 0, 9);
        mBlanked = false;
        mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
        mMonitorLed.updateDuty();
        saveToNvs();
    }

    void MonitorBrightnessController::cycleBrightness()
    {
        mCurrentBrightnessLevel = (mCurrentBrightnessLevel + 1) % 10;
        mBlanked = false;
        mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
        mMonitorLed.updateDuty();
        saveToNvs();
    }

    void MonitorBrightnessController::setState(ControlBoardPowerState state)
    {
        mCurrentPowerState = state;
        switch (state)
        {
        case ControlBoardPowerState::GOING_TO_SLEEP:
        case ControlBoardPowerState::SLEEP:
        case ControlBoardPowerState::GOING_INTO_DEEP_SLEEP:
        case ControlBoardPowerState::DEEPSLEEP:
        case ControlBoardPowerState::SHUTTING_DOWN:
        case ControlBoardPowerState::OFF:
            mSavedBrightnessLevel = mCurrentBrightnessLevel;
            ESP_LOGI(spTag, "Saved brightness level %d", mSavedBrightnessLevel);
            break;
        case ControlBoardPowerState::TURNING_ON:
        case ControlBoardPowerState::ON:
            mCurrentBrightnessLevel = mSavedBrightnessLevel;
            mBlanked = false;
            mMonitorLed.setDuty(mBrightnessLevels[mCurrentBrightnessLevel]);
            mMonitorLed.updateDuty();
            ESP_LOGI(spTag, "Restored brightness level %d", mCurrentBrightnessLevel);
            break;
        default:
            break;
        }
    }

    void MonitorBrightnessController::saveToNvs()
    {
        nvs_handle_t handle;
        if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) == ESP_OK)
        {
            nvs_set_i8(handle, kNvsKeyLevel, static_cast<int8_t>(mCurrentBrightnessLevel));
            nvs_commit(handle);
            nvs_close(handle);
            ESP_LOGI(spTag, "Saved brightness level %d to NVS", mCurrentBrightnessLevel);
        }
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
