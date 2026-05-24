#pragma once

#include "input/actions/buttonAction.hpp"
#include "app/ActionFactory.hpp"

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
}