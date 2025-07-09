#include "powerLed.hpp"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_ON LEDC_CHANNEL_0
#define LEDC_CHANNEL_STANDBY LEDC_CHANNEL_1 // Define a second channel for standby LED
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4096)                // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (4000) 
#define LED_OFF 0 // Define a constant for LED off state



namespace indicators
{

    PowerLed::PowerLed(gpio_num_t PIN_APP_ACTIVE_LED,gpio_num_t PIN_APP_STANDBY_LED)    {

    dutyCycle = 4096  ;
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};

    ledc_channel_config_t ledc_channel_on = {
        .gpio_num = PIN_APP_ACTIVE_LED,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_ON,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};

    ledc_channel_config_t ledc_channel_standBy = {
        .gpio_num = PIN_APP_STANDBY_LED,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_STANDBY,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};

        onLed.init(ledc_timer, ledc_channel_on);
        standBy.init(ledc_timer, ledc_channel_standBy); // Initialize the LED channels
    };
    PowerLed::~PowerLed()
    {
    };

    void PowerLed::setState(ControlBoardState state)
    {
        



        switch (state)
        {
        case ControlBoardState::Active:
            onLed.setDuty(dutyCycle); // 50% duty cycle
            onLed.updateDuty();
            
            standBy.setDuty( LED_OFF); // 0% duty cycle
            standBy.updateDuty();
            break;
        case ControlBoardState::Standby:
            standBy.setDuty(dutyCycle); // 50% duty cycle
            standBy.updateDuty();

            onLed.setDuty( LED_OFF); // 0% duty cycle
            onLed.updateDuty();
            break;
        default:
            // switch off all leds
            onLed.setDuty(LED_OFF); // 0% duty cycle
            standBy.setDuty( LED_OFF); // 0% duty cycle
            onLed.updateDuty();
            standBy.updateDuty();
            ESP_LOGW("PowerLed", "Unknown state: %d, turning off all LEDs", static_cast<int>(state));
            break;
        };
    };


    void PowerLed::setBrightness(int brightness)
    {
        brightness = (brightness < 0) ? 0 : (brightness > 100) ? 100 : brightness; // Clamp brightness to [0, 100]
        dutyCycle = (brightness * 4096) / 100; // Convert percentage to duty cycle for 13-bit resolution
    }

    void PowerLed::update()
    {
        // This function can be used to update the LED state if needed
        // For example, you might want to blink the LED or change its brightness
        // based on some conditions.
    };

};

