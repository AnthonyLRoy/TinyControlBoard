#pragma once
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"


namespace led
{

class LEDPWM
{
    private:
        ledc_timer_config_t  ledc_timerConfig;
        ledc_channel_config_t ledc_channelConfig;


    public:
     void init(ledc_timer_config_t timerConfig, ledc_channel_config_t channelConfig);
     esp_err_t setDuty(uint32_t duty);
     esp_err_t updateDuty();
};


}