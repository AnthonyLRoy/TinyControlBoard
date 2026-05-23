#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Concrete data-transfer object for the produce() → dispatch pipeline.
    /// All data fields are inherited from IAction.  execute() is intentionally
    /// a no-op: ActionProcessor creates purpose-built IAction subtypes via the
    /// factory for actual execution.
    struct Action : public IAction
    {
        bool requiresPowerOn() const override { return true; }
        void execute(controlSystem::ActionContext &) override {}
        Action() = default;
    };
}