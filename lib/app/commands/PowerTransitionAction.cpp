#include "app/commands/PowerTransitionAction.hpp"
#include "app/ActionContext.hpp"

namespace actions
{
    // Applies the requested power state transition and stores the resulting state back into the board runtime state.
    void PowerTransitionAction::execute(controlSystem::ActionContext &ctx)
    {
        ctx.systemState.powerState.store(ctx.powerHandler.handle(*this));
    }
}
