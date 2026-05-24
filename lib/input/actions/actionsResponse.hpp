#pragma once

#include "input/actions/IAction.hpp"

namespace actions
{
    /// Minimal no-op IAction implementation.
    /// Used in host-test helpers and UART dispatcher tests where a concrete
    /// IAction value is needed without going through ActionFactory.
    /// Not produced by any IActionSource in the live firmware pipeline.
    struct Action : public IAction
    {
        bool requiresPowerOn() const override { return true; }
        void execute(controlSystem::ActionContext &) override {}
        Action() = default;
    };
}