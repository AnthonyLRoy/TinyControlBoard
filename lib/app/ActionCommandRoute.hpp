#pragma once

namespace controlSystem
{
    /// The intrinsic routing category of an Action.
    /// Stored on every Action object so that ActionProcessor can dispatch
    /// without re-classifying the command at execution time.
    /// Note: IgnoreWhileNotOn is never stored on an Action; the power gate
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
