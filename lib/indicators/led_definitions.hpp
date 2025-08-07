#include "enums.hpp"  // Add this include for ControlBoardState

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_ON LEDC_CHANNEL_0
#define LEDC_CHANNEL_STANDBY LEDC_CHANNEL_1 // Define a second channel for standby LED
#define LEDC_CHANNEL_WORKSTATUS LEDC_CHANNEL_2 // Define a third channel for active LED
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT     // Set duty resolution to 13 bits
#define LEDC_DUTY (4096)                    // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (4000)
#define LED_OFF 0 // Define a constant for LED off state