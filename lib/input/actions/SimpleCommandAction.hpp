#pragma once

#include "input/actions/buttonAction.hpp"
#include "input/actions/actionsResponse.hpp"

namespace actions
{
    class SimpleCommandAction : public IActionSource
    {
    public:
        explicit SimpleCommandAction(CommandId commandId) : m_commandId(commandId) {}

        std::optional<Action> produce(bool isPressed) override
        {
            if (!isPressed)
                return std::nullopt;
            Action action;
            action.command = m_commandId;
            return action;
        }

    private:
        CommandId m_commandId;
    };
}