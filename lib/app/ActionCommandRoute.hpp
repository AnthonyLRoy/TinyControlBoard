#pragma once

namespace controlSystem
{
 /// defines the routing categories for action commands within the control system.
    /// in ActionProcessor handles that concern independently.
    enum class ActionCommandRoute
    {
        None,
        PowerStateTransition,
        IgnoreWhileNotOn,
        System,
        Relay,
        Brightness,
        UartDispatch,
    };
}
