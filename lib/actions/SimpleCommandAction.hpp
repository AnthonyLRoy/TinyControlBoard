#pragma once
#include "ButtonAction.hpp"
#include "actionsResponse.hpp"


namespace actions
{
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