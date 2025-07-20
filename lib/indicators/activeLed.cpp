#pragma once
#include "activeLed.hpp"

namespace indicators

{
    ActiveLed::ActiveLed(gpio_num_t PIN_APP_ACTIVE_LED)
    {
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_MODE,
            .duty_resolution = LEDC_DUTY_RES,
            .timer_num = LEDC_TIMER,
            .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
            .clk_cfg = LEDC_AUTO_CLK};

        ledc_channel_config_t ledc_channel_active = {
            .gpio_num = PIN_APP_ACTIVE_LED,
            .speed_mode = LEDC_MODE,
            .channel = LEDC_CHANNEL_WORKSTATUS,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER,
            .duty = 0, // Set duty to 0%
            .hpoint = 0};

        activeLed.init(ledc_timer, ledc_channel_active);
    };

    void ActiveLed::SetStatus(ControlBoardWorkingStatus newStatus)
    {
        switch (newStatus)
        {
        case ControlBoardWorkingStatus::doingWork:
            activeLed.setDuty(4096); // 100% duty cycle
            break;
        case ControlBoardWorkingStatus::Idle:
            activeLed.setDuty(2048); // 50% duty cycle
            break;
        case ControlBoardWorkingStatus::sleeping:
            activeLed.setDuty(1024); // 25% duty cycle
            break;
        case ControlBoardWorkingStatus::MaintenanceMode:
            activeLed.setDuty(512); // 12.5% duty cycle
            break;
        default:
            activeLed.setDuty(0); // 0% duty cycle for unknown status
            ESP_LOGW("ActiveLed", "Unknown status: %d, turning off LED", static_cast<int>(newStatus));
            break;
        }
        activeLed.updateDuty();
    };

    ActiveLed::~ActiveLed() {
        // Destructor can be used to clean up resources if needed
    };

    void ActiveLed::update() {
        // This function can be used to update the active LED state if needed
        // For example, you might want to blink the LED or change its brightness
        // based on some conditions.
    };
}