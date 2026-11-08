#include "app/ActionFactory.hpp"
#include "app/ActionCommandRoutingPolicy.hpp"
#include "app/commands/UartDispatchAction.hpp"
#include "app/commands/RelayAction.hpp"
#include "app/commands/PowerTransitionAction.hpp"
#include "app/commands/SystemAction.hpp"
#include "app/commands/BrightnessAction.hpp"

namespace controlSystem
{
    // Template specializations for each action route type

    template<>
    std::unique_ptr<actions::IAction> 
    createActionForRoute<ActionCommandRoute::PowerStateTransition>(CommandId command, uint16_t releaseTimeMs)
    {
        return std::make_unique<actions::PowerTransitionAction>(command, releaseTimeMs);
    }

    template<>
    std::unique_ptr<actions::IAction> 
    createActionForRoute<ActionCommandRoute::System>(CommandId command, uint16_t)
    {
        return std::make_unique<actions::SystemAction>(command);
    }

    template<>
    std::unique_ptr<actions::IAction> 
    createActionForRoute<ActionCommandRoute::Relay>(CommandId command, uint16_t)
    {
        return std::make_unique<actions::RelayAction>(command);
    }

    template<>
    std::unique_ptr<actions::IAction> 
    createActionForRoute<ActionCommandRoute::Brightness>(CommandId command, uint16_t)
    {
        return std::make_unique<actions::BrightnessAction>(command);
    }

    template<>
    std::unique_ptr<actions::IAction> 
    createActionForRoute<ActionCommandRoute::UartDispatch>(CommandId command, uint16_t)
    {
        return std::make_unique<actions::UartDispatchAction>(command);
    }

    template<>
    std::unique_ptr<actions::IAction> 
    createActionForRoute<ActionCommandRoute::None>(CommandId, uint16_t)
    {
        return nullptr;
    }

    template<>
    std::unique_ptr<actions::IAction> 
    createActionForRoute<ActionCommandRoute::IgnoreWhileNotOn>(CommandId command, uint16_t)
    {
        return std::make_unique<actions::UartDispatchAction>(command);
    }

    // Main factory dispatcher
    std::unique_ptr<actions::IAction> createAction(CommandId command, uint16_t releaseTimeMs)
    {
        const auto route = classifyCommand(command, ControlBoardPowerState::ON);

        switch (route)
        {
        case ActionCommandRoute::None:
            return createActionForRoute<ActionCommandRoute::None>(command, releaseTimeMs);
        case ActionCommandRoute::PowerStateTransition:
            return createActionForRoute<ActionCommandRoute::PowerStateTransition>(command, releaseTimeMs);
        case ActionCommandRoute::System:
            return createActionForRoute<ActionCommandRoute::System>(command, releaseTimeMs);
        case ActionCommandRoute::Relay:
            return createActionForRoute<ActionCommandRoute::Relay>(command, releaseTimeMs);
        case ActionCommandRoute::Brightness:
            return createActionForRoute<ActionCommandRoute::Brightness>(command, releaseTimeMs);
        case ActionCommandRoute::UartDispatch:
            return createActionForRoute<ActionCommandRoute::UartDispatch>(command, releaseTimeMs);
        case ActionCommandRoute::IgnoreWhileNotOn:
            return createActionForRoute<ActionCommandRoute::IgnoreWhileNotOn>(command, releaseTimeMs);
        default:
            return createActionForRoute<ActionCommandRoute::UartDispatch>(command, releaseTimeMs);
        }
    }
}
