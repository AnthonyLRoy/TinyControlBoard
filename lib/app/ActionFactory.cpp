#include "app/ActionFactory.hpp"
#include "app/ActionCommandRoutingPolicy.hpp"
#include "app/commands/UartDispatchAction.hpp"
#include "app/commands/RelayAction.hpp"
#include "app/commands/PowerTransitionAction.hpp"
#include "app/commands/SystemAction.hpp"
#include "app/commands/DisplayAction.hpp"
#include "app/commands/BrightnessAction.hpp"

namespace controlSystem
{
    std::unique_ptr<actions::IAction> createAction(CommandId command, uint16_t releaseTimeMs)
    {
        switch (classifyCommand(command, ControlBoardPowerState::ON))
        {
        case ActionCommandRoute::None:
            return nullptr;

        case ActionCommandRoute::PowerStateTransition:
            return std::make_unique<actions::PowerTransitionAction>(command, releaseTimeMs);

        case ActionCommandRoute::System:
            return std::make_unique<actions::SystemAction>(command);

        case ActionCommandRoute::Relay:
            return std::make_unique<actions::RelayAction>(command);

        case ActionCommandRoute::Display:
            return std::make_unique<actions::DisplayAction>(command);

        case ActionCommandRoute::Brightness:
            return std::make_unique<actions::BrightnessAction>(command);

        case ActionCommandRoute::UartDispatch:
        case ActionCommandRoute::IgnoreWhileNotOn: // never reached (ON passed above)
        default:
            return std::make_unique<actions::UartDispatchAction>(command);
        }
    }
}
