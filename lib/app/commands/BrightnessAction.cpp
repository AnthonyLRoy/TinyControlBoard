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
        if (command == CMD_SET_BRIGHTNESS_UP)
        {
            // For now, just set to a fixed level for testing; later, this will be parameterized
            ctx.brightnessController.setBrightnessUp(1);
        }
        if (command == CMD_SET_BRIGHTNESS_DOWN)
        {
            // For now, just set to a fixed level for testing; later, this will be parameterized
            ctx.brightnessController.setBrightnessDown(1);
        }

    }
}
