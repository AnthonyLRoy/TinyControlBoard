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
        ToggleAction() : m_state(false) {}

        ActionResponse execute(bool isPressed) override
        {
            ActionResponse response;
            if (isPressed)
            {
                m_state = !m_state;
                response.command = m_state ? CMD_ON : CMD_OFF;
            }
            const char *p_commandName = controlSystem::getCommandNameById(response.command);
            ESP_LOGI("Toggle_Action   ", "Toggle action executed: %s, new state: %s", p_commandName, m_state ? "ON" : "OFF");
            return response;
        }

    private:
        bool m_state;
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
        TimedAction() : m_pressStartUs(0) {}

        ActionResponse execute(bool isPressed) override
        {
            ActionResponse response;
            if (isPressed)
            {
                m_pressStartUs = esp_timer_get_time();
                ESP_LOGI("Timed_Action    ", "Press start timestamp: %" PRIi64 " us", m_pressStartUs);
            }
            else
            {
                ESP_LOGI("Timed_Action    ", "Press start timestamp (for validation): %" PRIi64 " us", m_pressStartUs);
                const int64_t durationUs = esp_timer_get_time() - m_pressStartUs;
                ESP_LOGI("Timed_Action    ", "Button was pressed for %" PRIu64 " us", durationUs);
                response.releaseTimeMillis = static_cast<uint32_t>(durationUs / 1000);
                response.command = CMD;
            }
            return response;
        }

    private:
        int64_t m_pressStartUs;
    };
}