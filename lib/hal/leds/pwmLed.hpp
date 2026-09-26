#pragma once

#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

namespace led
{
    class LedPwm
    {
    private:
        ledc_timer_config_t m_ledcTimerConfig;
        ledc_channel_config_t m_ledcChannelConfig;

    public:
        // Configures the LEDC timer and channel for the PWM output channel.
        void init(ledc_timer_config_t timerConfig, ledc_channel_config_t channelConfig);
        // Sets the output duty cycle for the PWM LED channel.
        esp_err_t setDuty(uint32_t duty);
        // Applies the current PWM configuration to the hardware timer/channel.
        esp_err_t updateDuty();
    };
}