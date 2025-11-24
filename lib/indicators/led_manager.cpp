#include "led_manager.hpp"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4
#define PIN_WORKING_STATUS_LED GPIO_NUM_48
#define STP_LEDS_BRIGHTNESS_LEVEL  GPIO_NUM_21

namespace indicators {

    static ActiveLed activeLed(PIN_WORKING_STATUS_LED,LEDC_CHANNEL_0);
    static ActiveLed buttonLeds(STP_LEDS_BRIGHTNESS_LEVEL,LEDC_CHANNEL_1);
    static PowerLed powerLed(PIN_APP_ACTIVE_LED,LEDC_CHANNEL_ON, PIN_APP_STANDBY_LED,LEDC_CHANNEL_STANDBY);

    ActiveLed& getActiveLed() {
        return activeLed;
    }

    PowerLed& getPowerLed() {
        if (!powerLed.started)
        {
            powerLed.init();
        }
        return powerLed;
    }

    ActiveLed& getButtonLed() {
        return buttonLeds;
    }
}