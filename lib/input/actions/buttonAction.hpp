#pragma once

#include "input/actions/actionsResponse.hpp"
#include <optional>

namespace actions
{
    /// Source of Action objects driven by button/rotary events.
    /// produce() returns nullopt when the event generates no pipeline action
    /// (e.g. press-down for a timed action, or release for a simple command).
    class IActionSource
    {
    public:
        virtual ~IActionSource() = default;
        virtual std::optional<Action> produce(bool isPressed) = 0;
    };
}