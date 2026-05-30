#include "app/commands/PowerTransitionAction.hpp"
#include "app/ActionContext.hpp"

namespace actions
{
    void PowerTransitionAction::execute(controlSystem::ActionContext &ctx)
    {
        ctx.systemState.powerState.store(ctx.powerHandler.handle(*this));
    }
}
