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

   void LEDPWM::init(ledc_timer_config_t timerConfig, ledc_channel_config_t channelConfig)
   {

      ledc_timerConfig = timerConfig;
      ledc_channelConfig = channelConfig;

      ESP_ERROR_CHECK(ledc_timer_config(&ledc_timerConfig));
      ESP_ERROR_CHECK(ledc_channel_config(&ledc_channelConfig));
   }
   esp_err_t LEDPWM::setDuty(uint32_t duty)
   {
      return ledc_set_duty(ledc_timerConfig.speed_mode, ledc_channelConfig.channel, duty);
   }

   esp_err_t LEDPWM::updateDuty()
   {
      return ledc_update_duty(ledc_timerConfig.speed_mode, ledc_channelConfig.channel);
   }
}




