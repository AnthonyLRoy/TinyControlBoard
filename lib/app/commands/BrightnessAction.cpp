#include "app/commands/BrightnessAction.hpp"
#include "app/ActionContext.hpp"

namespace actions
{
    void BrightnessAction::execute(controlSystem::ActionContext &ctx)
    {
        switch (command)
        {
        case CMD_TOGGLE_DISPLAY:
            ctx.brightnessController.toggleDisplayOffOn();
            break;
        case CMD_CYCLE_BRIGHTNESS:
            ctx.brightnessController.cycleBrightness();
            break;
        default:
            break;
        }
    }
}
