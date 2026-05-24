#pragma once

#include "input/actions/IAction.hpp"
#include <memory>

namespace actions
{
    /// Source of executable IAction objects driven by button/rotary events.
    /// produce() returns nullptr when the event generates no pipeline action
    /// (e.g. press-down for a timed action, or release for a simple command).
    class IActionSource
    {
    public:
        virtual ~IActionSource() = default;
        virtual std::unique_ptr<IAction> produce(bool isPressed) = 0;
    };
}