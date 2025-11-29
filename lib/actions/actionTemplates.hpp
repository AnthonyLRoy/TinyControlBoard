#pragma once
#include "ButtonAction.hpp"
#include "actionsResponse.hpp"
#include <esp_timer.h>
#include "controlSytemHelpers.hpp"
#include "esp_log.h"
namespace actions
{

    template <commandID CMD_ON, commandID CMD_OFF>
    class ToggleAction : public ButtonAction
    {

    public:
        ToggleAction() : state_(false) {}

        actionResponse execute(bool pressed) override
        {
            actionResponse response;
            if (pressed)
            {
                state_ = !state_;
                response.command = state_ ? CMD_ON : CMD_OFF;
                response.KeepLedActive = state_;
            }
            const char* commandName = controlSystem::getCommandNameById(response.command);
            ESP_LOGI("ToggleAction", "Executed toggle action: %s, New State: %s", commandName, state_ ? "ON" : "OFF");
            return response;
        }

    private:
        bool state_;
    };

    template <commandID ROTATE_DIRECTION>
    class RotaryAction : public ButtonAction
    {
        actionResponse execute(bool IsLeft) override
        {
            actionResponse response;
                response.command = CMD_ROTARY_ACTION;
                response.parameters[0] = ROTATE_DIRECTION;
            return response;
        }
    };

    /**
     * Momentary button that only sends a command when pressed.
     * CMD: Command to send
     */
    template <commandID CMD>
    class MomentaryAction : public ButtonAction
    {
    public:
        MomentaryAction() = default;

        actionResponse execute(bool pressed) override
        {
            actionResponse response;
            if (pressed)
                response.command = CMD;
            return response;
        }
    };

  
    template <commandID CMD>
    class TimedAction : public ButtonAction
    {
    public:
        TimedAction() : pressStartUs_(0) {}

        actionResponse execute(bool pressed) override
        {
            actionResponse response;
            if (pressed)
            {
                pressStartUs_ = esp_timer_get_time();
            }
            else
            {
                const int64_t durationUs = esp_timer_get_time() - pressStartUs_;
                response.releaseTimeMilliSecs = static_cast<uint32_t>(durationUs / 1000);
                response.command = CMD;
            }
            return response;
        }

    private:
        int64_t pressStartUs_;
    };
}
