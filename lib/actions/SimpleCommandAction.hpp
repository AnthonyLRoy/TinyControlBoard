#pragma once
#include "buttonAction.hpp"
#include "actionsResponse.hpp"


namespace actions
{
/**
 * @brief SimpleCommandAction triggers a specific command when a button is pressed.
 *
 * This class implements the ButtonAction interface. When the associated button is pressed,
 * it sets the ActionResponse command to the provided CommandId. It is useful for mapping
 * simple button presses directly to command executions without additional logic.
 */
class SimpleCommandAction : public ButtonAction {
public:
    explicit SimpleCommandAction(CommandId commandId) : mCommandId(commandId) {}

    ActionResponse execute(bool isPressed) override {
        ActionResponse response;
        if (isPressed)
            response.command = mCommandId;
        return response;
    }

private:
    CommandId mCommandId;
};
}