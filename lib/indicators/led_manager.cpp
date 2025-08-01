#include "led_manager.hpp"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4
#define PIN_WORKING_STATUS_LED GPIO_NUM_48
#define STP_LEDS_BRIGHTNESS_LEVEL  GPIO_NUM_21

namespace indicators {

    static ActiveLed activeLed(PIN_WORKING_STATUS_LED);
    static ActiveLed buttonLed(STP_LEDS_BRIGHTNESS_LEVEL);
    static PowerLed powerLed(PIN_APP_ACTIVE_LED, PIN_APP_STANDBY_LED);

    ActiveLed& getActiveLed() {
        return activeLed;
    }

    PowerLed& getPowerLed() {
        return powerLed;
    }

    ActiveLed& getButtonLed() {
        return buttonLed;
    }
}