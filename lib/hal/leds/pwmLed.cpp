#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"

#include "pwmLed.hpp"

namespace led
{
   void LedPwm::init(ledc_timer_config_t timerConfig, ledc_channel_config_t channelConfig)
   {
      ESP_LOGI("Led_PWM         ", "Initializing Led_PWM with channel: %d", channelConfig.channel);

      m_ledcTimerConfig = timerConfig;
      m_ledcChannelConfig = channelConfig;

      ESP_ERROR_CHECK(ledc_timer_config(&m_ledcTimerConfig));
      ESP_ERROR_CHECK(ledc_channel_config(&m_ledcChannelConfig));
   }

   esp_err_t LedPwm::setDuty(uint32_t duty)
   {
      return ledc_set_duty(m_ledcTimerConfig.speed_mode, m_ledcChannelConfig.channel, duty);
   }

   esp_err_t LedPwm::updateDuty()
   {
      return ledc_update_duty(m_ledcTimerConfig.speed_mode, m_ledcChannelConfig.channel);
   }
}