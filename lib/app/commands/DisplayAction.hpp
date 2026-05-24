#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Blanks or unblanks the connected display.
    class DisplayAction : public IAction
    {
    public:
        explicit DisplayAction(CommandId cmd) { command = cmd; }

        bool requiresPowerOn() const override { return true; }
        void execute(controlSystem::ActionContext &ctx) override;
    };
}
