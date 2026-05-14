#pragma once

#include "input/actions/buttonAction.hpp"
#include "input/actions/actionsResponse.hpp"
#include <esp_timer.h>
#include "support/controlSystemHelpers.hpp"
#include "esp_log.h"

namespace actions
{
    template <CommandId CMD_ON, CommandId CMD_OFF>
    class ToggleAction : public ButtonAction
    {
    public:
        ToggleAction() : mState(false) {}

        ActionResponse execute(bool isPressed) override
        {
            ActionResponse response;
            response.keepLedActive = mState;
            if (isPressed)
            {
                mState = !mState;
                response.command = mState ? CMD_ON : CMD_OFF;
                response.keepLedActive = mState;
            }
            const char *pCommandName = controlSystem::getCommandNameById(response.command);
            ESP_LOGI("Toggle_Action   ", "Executed toggle action: %s, New State: %s", pCommandName, mState ? "ON" : "OFF");
            return response;
        }

    private:
        bool mState;
    };

    template <CommandId ROTATE_DIRECTION>
    class RotaryAction : public ButtonAction
    {
        ActionResponse execute(bool isLeft) override
        {
            ActionResponse response;
            response.command = CMD_ROTARY_ACTION;
            response.parameters[0] = isLeft ? 0 : 1;
            return response;
        }
    };

    template <CommandId CMD>
    class MomentaryAction : public ButtonAction
    {
    public:
        MomentaryAction() = default;

        ActionResponse execute(bool isPressed) override
        {
            ActionResponse response;
            if (isPressed)
            {
                response.command = CMD;
            }
            return response;
        }
    };

    template <CommandId CMD>
    class TimedAction : public ButtonAction
    {
    public:
        TimedAction() : mPressStartUs(0) {}

        ActionResponse execute(bool isPressed) override
        {
            ActionResponse response;
            if (isPressed)
            {
                mPressStartUs = esp_timer_get_time();
                ESP_LOGI("Timed_Action    ", "inital value at %" PRIi64 " us", mPressStartUs);
            }
            else
            {
                ESP_LOGI("Timed_Action    ", "Validate at %" PRIi64 " us", mPressStartUs);
                const int64_t durationUs = esp_timer_get_time() - mPressStartUs;
                ESP_LOGI("Timed_Action    ", "Button was pressed for %" PRIu64 " us", durationUs);
                response.releaseTimeMillis = static_cast<uint32_t>(durationUs / 1000);
                response.command = CMD;
            }
            return response;
        }

    private:
        int64_t mPressStartUs;
    };
}