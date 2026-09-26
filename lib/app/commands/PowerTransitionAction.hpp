#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Drives a power-state transition such as power-on, sleep, or deep-sleep.
    /// It can be used even before the system is active because it is the action that changes the state.
    class PowerTransitionAction : public IAction
    {
    public:
        // Creates a power transition action for the given command and release timing.
        PowerTransitionAction(CommandId cmd, uint16_t releaseMs)
        {
            command = cmd;
            releaseTimeMillis = releaseMs;
        }

        // Power transitions are allowed even while the board is off because they initiate the on-state itself.
        bool requiresPowerOn() const override { return false; }
        // Executes the transition and stores the resulting state in the board runtime data.
        void execute(controlSystem::ActionContext &ctx) override;
    };
}
