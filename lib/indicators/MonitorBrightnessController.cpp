#include "monitorBrightnessController.hpp"
#include <cmath>
#include <algorithm>

namespace
{
    constexpr const char *k_nvsKeyLevel = "level";
}

static constexpr const char *k_logTag = "Monitor_Bright  ";

namespace indicators
{
    namespace
    {
        uint32_t getDutyForBrightnessLevel(int brightnessLevel)
        {
            const int clampedLevel = std::clamp(brightnessLevel, 0, 9);
            constexpr uint32_t k_brightnessDuties[10] = {0, 500, 750, 1000, 1250, 1500, 2000, 3000, 3500, 4000};
            return k_brightnessDuties[clampedLevel];
        }
    }

    MonitorBrightnessController::~MonitorBrightnessController()
    {
    }

    void MonitorBrightnessController::init()
    {
        ESP_LOGI(k_logTag, "Initializing MonitorBrightnessController hardware");
        ESP_LOGI(k_logTag, "monitorPin: %d, pwmChannel: %d", m_monitorPin, m_pwmChannel);

        // LEDC timer
        ledc_timer_config_t timer = {};
        timer.speed_mode = LEDC_MODE;
        timer.duty_resolution = LEDC_DUTY_RES;
        timer.timer_num = LEDC_TIMER_0;
        timer.freq_hz = 4000;
        timer.clk_cfg = LEDC_AUTO_CLK;
        if (ledc_timer_config(&timer) != ESP_OK) {
            ESP_LOGE(k_logTag, "Failed to configure LEDC timer");
        }

        // Active LED
        ledc_channel_config_t activeCfg = {};
        activeCfg.channel = m_pwmChannel;
        activeCfg.duty = 0;
        activeCfg.gpio_num = m_monitorPin;
        activeCfg.speed_mode = LEDC_MODE;
        activeCfg.hpoint = 0;
        activeCfg.timer_sel = LEDC_TIMER_0;
        if (ledc_channel_config(&activeCfg) != ESP_OK) {
            ESP_LOGE(k_logTag, "Failed to configure LEDC channel");
        }

        // Init PWM wrappers
        m_monitorLed.init(timer, activeCfg);

        // Load persisted brightness level from NVS
        int8_t savedLevel = static_cast<int8_t>(m_currentBrightnessLevel);
        if (m_nvsStorage.readInt8(k_nvsKeyLevel, savedLevel))
        {
            m_currentBrightnessLevel = std::clamp(static_cast<int>(savedLevel), 0, 9);
            ESP_LOGI(k_logTag, "Restored brightness level %d from NVS", m_currentBrightnessLevel);
        }
        else
        {
            // First boot — write the default so it exists next time
            m_nvsStorage.writeInt8(k_nvsKeyLevel, static_cast<int8_t>(m_currentBrightnessLevel));
            ESP_LOGI(k_logTag, "NVS: first boot, wrote default brightness %d", m_currentBrightnessLevel);
        }
        m_savedBrightnessLevel = m_currentBrightnessLevel;
        m_displayOffSavedBrightnessLevel = m_currentBrightnessLevel;
        m_displayOffActive = false;
        m_blanked = false;

        // Apply the loaded level
        m_monitorLed.setDuty(m_brightnessLevels[m_currentBrightnessLevel]);
        m_monitorLed.updateDuty();

        m_started = true;
    }

    void MonitorBrightnessController::changeBrightnessLevel(int change)
    {
        if (m_displayOffActive)
        {
            ESP_LOGI(k_logTag, "Ignoring brightness change while Display Off/On is active");
            return;
        }

        ESP_LOGI(k_logTag, "Changing brightness level by %d", change);
        m_currentBrightnessLevel = std::clamp(m_currentBrightnessLevel + change, 0, 9);
        m_blanked = false;
        m_monitorLed.setDuty(m_brightnessLevels[m_currentBrightnessLevel]);
        m_monitorLed.updateDuty();
        m_nvsStorage.writeInt8(k_nvsKeyLevel, static_cast<int8_t>(m_currentBrightnessLevel));
        ESP_LOGI(k_logTag, "Saved brightness level %d to NVS", m_currentBrightnessLevel);
    }

    void MonitorBrightnessController::cycleBrightness()
    {
        if (m_displayOffActive)
        {
            ESP_LOGI(k_logTag, "Ignoring SetBrightness while Display Off/On is active");
            return;
        }

        m_currentBrightnessLevel = (m_currentBrightnessLevel + 1) % 10;
        m_blanked = false;
        m_monitorLed.setDuty(m_brightnessLevels[m_currentBrightnessLevel]);
        m_monitorLed.updateDuty();
        m_nvsStorage.writeInt8(k_nvsKeyLevel, static_cast<int8_t>(m_currentBrightnessLevel));
        ESP_LOGI(k_logTag, "Saved brightness level %d to NVS", m_currentBrightnessLevel);
    }

    void MonitorBrightnessController::setState(ControlBoardPowerState state)
    {
        m_currentPowerState = state;
        switch (state)
        {
        case ControlBoardPowerState::GOING_TO_SLEEP:
        case ControlBoardPowerState::SLEEP:
        case ControlBoardPowerState::GOING_INTO_DEEP_SLEEP:
        case ControlBoardPowerState::DEEPSLEEP:
        case ControlBoardPowerState::SHUTTING_DOWN:
        case ControlBoardPowerState::OFF:
            clearDisplayOffMode();
            m_savedBrightnessLevel = m_currentBrightnessLevel;
            ESP_LOGI(k_logTag, "Saved brightness level %d", m_savedBrightnessLevel);
            break;
        case ControlBoardPowerState::TURNING_ON:
        case ControlBoardPowerState::ON:
            m_currentBrightnessLevel = m_savedBrightnessLevel;
            m_blanked = false;
            m_monitorLed.setDuty(m_brightnessLevels[m_currentBrightnessLevel]);
            m_monitorLed.updateDuty();
            ESP_LOGI(k_logTag, "Restored brightness level %d", m_currentBrightnessLevel);
            break;
        default:
            break;
        }
    }

    void MonitorBrightnessController::toggleDisplayOffOn()
    {
        if (!m_displayOffActive)
        {
            m_displayOffSavedBrightnessLevel = m_currentBrightnessLevel;
            m_displayOffActive = true;
            m_blanked = true;
            m_monitorLed.setDuty(0);
            m_monitorLed.updateDuty();
            ESP_LOGI(k_logTag, "Display Off/On enabled, brightness temporarily forced to 0");
            return;
        }

        m_displayOffActive = false;
        m_currentBrightnessLevel = std::clamp(m_displayOffSavedBrightnessLevel, 0, 9);
        m_blanked = false;
        m_monitorLed.setDuty(m_brightnessLevels[m_currentBrightnessLevel]);
        m_monitorLed.updateDuty();
        ESP_LOGI(k_logTag, "Display Off/On disabled, restored brightness level %d", m_currentBrightnessLevel);
    }

    void MonitorBrightnessController::clearDisplayOffMode()
    {
        if (!m_displayOffActive)
        {
            return;
        }

        m_displayOffActive = false;
        m_currentBrightnessLevel = std::clamp(m_displayOffSavedBrightnessLevel, 0, 9);
        m_blanked = false;
        m_monitorLed.setDuty(m_brightnessLevels[m_currentBrightnessLevel]);
        m_monitorLed.updateDuty();
        ESP_LOGI(k_logTag, "Cleared Display Off/On before power transition, restored brightness level %d", m_currentBrightnessLevel);
    }

    void MonitorBrightnessController::setBlanked(bool blanked)
    {
        m_blanked = blanked;
        const uint32_t duty = blanked ? 0 : getDutyForBrightnessLevel(m_currentBrightnessLevel);
        ESP_LOGI(k_logTag, "%s monitor backlight", blanked ? "Blanking" : "Restoring");
        m_monitorLed.setDuty(duty);
        m_monitorLed.updateDuty();
    }


} // namespace indicators
