#pragma once

#include "input/actions/buttonAction.hpp"
#include "app/ActionFactory.hpp"
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

        std::unique_ptr<IAction> produce(bool isPressed) override
        {
            if (!isPressed)
                return nullptr;
            m_state = !m_state;
            CommandId cmd = m_state ? CMD_ON : CMD_OFF;
            const char *p_commandName = controlSystem::getCommandNameById(cmd);
            ESP_LOGI("Toggle_Action   ", "Toggle action executed: %s, new state: %s", p_commandName, m_state ? "ON" : "OFF");
            return controlSystem::createAction(cmd);
        }

    private:
        bool m_state;
    };

    template <CommandId ROTATE_DIRECTION>
    class RotaryAction : public IActionSource
    {
    public:
        std::unique_ptr<IAction> produce(bool isLeft) override
        {
            auto iaction = controlSystem::createAction(CMD_ROTARY_ACTION);
            iaction->parameters[0] = isLeft ? 0 : 1;
            return iaction;
        }
    };

    template <CommandId CMD>
    class MomentaryAction : public IActionSource
    {
    public:
        MomentaryAction() = default;

        std::unique_ptr<IAction> produce(bool isPressed) override
        {
            if (!isPressed)
                return nullptr;
            return controlSystem::createAction(CMD);
        }
    };

    template <CommandId CMD>
    class TimedAction : public IActionSource
    {
    public:
        TimedAction() : m_pressStartUs(0) {}

        std::unique_ptr<IAction> produce(bool isPressed) override
        {
            if (isPressed)
            {
                m_pressStartUs = esp_timer_get_time();
                ESP_LOGI("Timed_Action    ", "Press start timestamp: %" PRIi64 " us", m_pressStartUs);
                return nullptr;
            }
            ESP_LOGI("Timed_Action    ", "Press start timestamp (for validation): %" PRIi64 " us", m_pressStartUs);
            const int64_t durationUs = esp_timer_get_time() - m_pressStartUs;
            ESP_LOGI("Timed_Action    ", "Button was pressed for %" PRIu64 " us", durationUs);
            uint16_t releaseMs = static_cast<uint16_t>(durationUs / 1000);
            return controlSystem::createAction(CMD, releaseMs);
        }

    private:
        int64_t m_pressStartUs;
    };
}