#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"

#include "indicators/pwm/pwmLed.hpp"

namespace led
{
   void LedPwm::init(ledc_timer_config_t timerConfig, ledc_channel_config_t channelConfig)
   {
      ESP_LOGI("LEDPWM", "Initializing LEDPWM with channel: %d", channelConfig.channel);

      mLedcTimerConfig = timerConfig;
      mLedcChannelConfig = channelConfig;

      ESP_ERROR_CHECK(ledc_timer_config(&mLedcTimerConfig));
      ESP_ERROR_CHECK(ledc_channel_config(&mLedcChannelConfig));
   }

   esp_err_t LedPwm::setDuty(uint32_t duty)
   {
      return ledc_set_duty(mLedcTimerConfig.speed_mode, mLedcChannelConfig.channel, duty);
   }

   esp_err_t LedPwm::updateDuty()
   {
      return ledc_update_duty(mLedcTimerConfig.speed_mode, mLedcChannelConfig.channel);
   }
}