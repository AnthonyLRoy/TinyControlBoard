#pragma once

#include "input/actions/buttonAction.hpp"
#include "input/actions/actionsResponse.hpp"

namespace actions
{
    class SimpleCommandAction : public ButtonAction
    {
    public:
        explicit SimpleCommandAction(CommandId commandId) : m_commandId(commandId) {}

        ActionResponse execute(bool isPressed) override
        {
            ActionResponse response;
            if (isPressed)
            {
                response.command = m_commandId;
            }
            return response;
        }

    private:
        CommandId m_commandId;
    };
}