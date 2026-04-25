#pragma once

#include "input/actions/buttonAction.hpp"
#include "input/actions/actionsResponse.hpp"

namespace actions
{
    class SimpleCommandAction : public ButtonAction
    {
    public:
        explicit SimpleCommandAction(CommandId commandId) : mCommandId(commandId) {}

        ActionResponse execute(bool isPressed) override
        {
            ActionResponse response;
            if (isPressed)
            {
                response.command = mCommandId;
            }
            return response;
        }

    private:
        CommandId mCommandId;
    };
}