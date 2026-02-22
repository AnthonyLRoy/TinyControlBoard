#pragma once
#include "ButtonAction.hpp"
#include "actionsResponse.hpp"


namespace actions
{
/**
 * @brief SimpleCommandAction triggers a specific command when a button is pressed.
 *
 * This class implements the ButtonAction interface. When the associated button is pressed,
 * it sets the actionResponse's command to the provided commandID. It is useful for mapping
 * simple button presses directly to command executions without additional logic.
 */
class SimpleCommandAction : public ButtonAction {
public:
    explicit SimpleCommandAction(commandID cmd) : cmd_(cmd) {}

    actionResponse execute(bool pressed) override {
        actionResponse response;
        if (pressed)
            response.command = cmd_;
        return response;
    }

private:
    commandID cmd_;
};
}