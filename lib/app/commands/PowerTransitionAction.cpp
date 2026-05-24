#include "app/commands/PowerTransitionAction.hpp"
#include "app/ActionContext.hpp"
#include "indicators/ledManager.hpp"

namespace actions
{
    void PowerTransitionAction::execute(controlSystem::ActionContext &ctx)
    {
        ctx.powerHandler.handle(*this);
        ctx.systemState.powerState.store(indicators::getPowerLed().getState());
    }
}
