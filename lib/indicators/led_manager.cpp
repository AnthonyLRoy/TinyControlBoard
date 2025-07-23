#include "led_manager.hpp"

#define PIN_APP_ACTIVE_LED GPIO_NUM_3
#define PIN_APP_STANDBY_LED GPIO_NUM_4
#define PIN_WORKING_STATUS_LED GPIO_NUM_48

namespace indicators {

    static ActiveLed activeLed(PIN_WORKING_STATUS_LED);
    static PowerLed powerLed(PIN_APP_ACTIVE_LED, PIN_APP_STANDBY_LED);

    ActiveLed& getActiveLed() {
        return activeLed;
    }

    PowerLed& getPowerLed() {
        return powerLed;
    }

}