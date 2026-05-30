#include "app/commands/BrightnessAction.hpp"
#include "app/ActionContext.hpp"

namespace actions
{
    void BrightnessAction::execute(controlSystem::ActionContext &ctx)
    {
        if (command == CMD_TOGGLE_DISPLAY)
        {
            ctx.brightnessController.toggleDisplayOffOn();
            return;
        }

        if (command == CMD_CYCLE_BRIGHTNESS)
        {
            ctx.brightnessController.cycleBrightness();
        }
    }
}
