#pragma once

#include "input/actions/buttonAction.hpp"
#include "input/actions/actionsResponse.hpp"
#include <esp_timer.h>
#include "support/controlSystemHelpers.hpp"
#include "esp_log.h"

namespace actions
{
    template <CommandId CMD_ON, CommandId CMD_OFF>
    class ToggleAction : public IActionSource
    {
    public:
        ToggleAction() : m_state(false) {}

        std::optional<Action> produce(bool isPressed) override
        {
            if (!isPressed)
                return std::nullopt;
            m_state = !m_state;
            Action action;
            action.command = m_state ? CMD_ON : CMD_OFF;
            const char *p_commandName = controlSystem::getCommandNameById(action.command);
            ESP_LOGI("Toggle_Action   ", "Toggle action executed: %s, new state: %s", p_commandName, m_state ? "ON" : "OFF");
            return action;
        }

    private:
        bool m_state;
    };

    template <CommandId ROTATE_DIRECTION>
    class RotaryAction : public IActionSource
    {
    public:
        std::optional<Action> produce(bool isLeft) override
        {
            Action action;
            action.command = CMD_ROTARY_ACTION;
            action.parameters[0] = isLeft ? 0 : 1;
            return action;
        }
    };

    template <CommandId CMD>
    class MomentaryAction : public IActionSource
    {
    public:
        MomentaryAction() = default;

        std::optional<Action> produce(bool isPressed) override
        {
            if (!isPressed)
                return std::nullopt;
            Action action;
            action.command = CMD;
            return action;
        }
    };

    template <CommandId CMD>
    class TimedAction : public IActionSource
    {
    public:
        TimedAction() : m_pressStartUs(0) {}

        std::optional<Action> produce(bool isPressed) override
        {
            if (isPressed)
            {
                m_pressStartUs = esp_timer_get_time();
                ESP_LOGI("Timed_Action    ", "Press start timestamp: %" PRIi64 " us", m_pressStartUs);
                return std::nullopt;
            }
            ESP_LOGI("Timed_Action    ", "Press start timestamp (for validation): %" PRIi64 " us", m_pressStartUs);
            const int64_t durationUs = esp_timer_get_time() - m_pressStartUs;
            ESP_LOGI("Timed_Action    ", "Button was pressed for %" PRIu64 " us", durationUs);
            Action action;
            action.command = CMD;
            action.releaseTimeMillis = static_cast<uint32_t>(durationUs / 1000);
            return action;
        }

    private:
        int64_t m_pressStartUs;
    };
}