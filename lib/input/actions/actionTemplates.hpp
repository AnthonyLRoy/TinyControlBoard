#pragma once

#include "input/actions/buttonAction.hpp"
#include "app/ActionFactory.hpp"
#include "protocol/commandCatalog.hpp"
#if __has_include(<esp_timer.h>)
#include <esp_timer.h>
#include "esp_log.h"
#else
#ifndef ESP_LOGI
#define ESP_LOGI(tag, fmt, ...)
#endif
#endif

namespace actions
{
    class SimpleCommandAction : public IActionSource
    {
    public:
        explicit SimpleCommandAction(CommandId commandId) : m_commandId(commandId) {}

        std::unique_ptr<IAction> produce(bool isPressed) override
        {
            if (!isPressed)
                return nullptr;
            return controlSystem::createAction(m_commandId);
        }

    private:
        CommandId m_commandId;
    };

    /// Runtime equivalent of ToggleAction<CMD_ON, CMD_OFF> — stores commands as member values.
    /// Use this when constructing from a registration table rather than explicit template params.
    class DynamicToggleAction : public IActionSource
    {
    public:
        DynamicToggleAction(CommandId cmdOn, CommandId cmdOff)
            : m_cmdOn(cmdOn), m_cmdOff(cmdOff), m_state(false) {}

        void resetState() override
        {
            m_state = false;
        }

        // Keeps this action's next ON/OFF press in sync after a remote (BLE app)
        // toggle, so a following physical press doesn't emit the stale command.
        void syncExternalState(bool enabled) override
        {
            m_state = enabled;
        }

        std::unique_ptr<IAction> produce(bool isPressed) override
        {
            if (!isPressed)
                return nullptr;
            m_state = !m_state;
            const CommandId cmd = m_state ? m_cmdOn : m_cmdOff;
            const char *p_commandName = controlSystem::getCommandNameById(cmd);
            ESP_LOGI("Toggle_Action   ", "Toggle action executed: %s, new state: %s",
                     p_commandName, m_state ? "ON" : "OFF");
            return controlSystem::createAction(cmd);
        }

    private:
        CommandId m_cmdOn;
        CommandId m_cmdOff;
        bool      m_state;
    };

    /// Runtime equivalent of TimedAction<CMD> — stores the command as a member value.
    class DynamicTimedAction : public IActionSource
    {
    public:
        explicit DynamicTimedAction(CommandId cmd) : m_cmd(cmd), m_pressStartUs(0) {}

        std::unique_ptr<IAction> produce(bool isPressed) override
        {
#if __has_include(<esp_timer.h>)
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
            return controlSystem::createAction(m_cmd, releaseMs);
#else
            (void)isPressed;
            return nullptr;
#endif
        }

    private:
        CommandId m_cmd;
        int64_t   m_pressStartUs;
    };

    /// Non-template equivalent of RotaryAction<> — the original template parameter was unused.
    class DynamicRotaryAction : public IActionSource
    {
    public:
        std::unique_ptr<IAction> produce(bool isLeft) override
        {
            auto iaction = controlSystem::createAction(CMD_ROTARY_ACTION);
            iaction->parameters[0] = isLeft ? 0 : 1;
            return iaction;
        }
    };
}