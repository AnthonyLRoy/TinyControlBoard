#pragma once
#include "driver/ledc.h"
#include "pwmLed.hpp" // Ensure this header defines the LED class
#include "led_definitions.hpp"
#include "PowerStateManager.hpp"
namespace indicators
{
    class PowerLed
    {
    public:
        PowerLed(gpio_num_t PIN_APP_ACTIVE_LED,ledc_channel_t OnChannel, gpio_num_t PIN_APP_STANDBY_LED,ledc_channel_t StandbyChannel);
        ~PowerLed();

        // Set the power LED state
        void setState(ControlBoardPowerState state);
        ControlBoardPowerState getState() const;
        void setBrightness(int brightness);

        // Update the power LED state
        void update();

    private:
        led::LEDPWM onLed;  // LED for power on indication
        led::LEDPWM standByLed; // LED for power off indication
        int dutyCycle = 4096; // Max duty cycle for 13-bit resolution
        
        // Current power state
        ControlBoardPowerState currentPowerState = ControlBoardPowerState::OFF;
        
        // Flashing control variables
        bool isOnLedFlashing = false;
        bool isStandByLedFlashing = false;
        uint64_t lastOnFlashTime = 0;
        uint64_t lastStandByFlashTime = 0;
        bool onFlashState = false;
        bool standByFlashState = false;
        uint32_t onFlashInterval = 500; // Flash interval in ms
        uint32_t standByFlashInterval = 500; // Flash interval in ms
        int mediumDutyCycle = 2048; // 50% of max (4096)
        int lowDutyCycle = 1024; // 25% of max (4096)
    };
}