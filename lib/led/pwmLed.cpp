#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"


#include "pwmLed.hpp"

namespace led
{

   /* Warning:
    * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2, ESP32P4 targets,
    * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
    * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
    */

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




