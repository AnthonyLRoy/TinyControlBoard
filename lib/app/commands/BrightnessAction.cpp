#include "app/commands/BrightnessAction.hpp"
#include "app/ActionContext.hpp"
#include "indicators/ledManager.hpp"

namespace actions
{
    void BrightnessAction::execute(controlSystem::ActionContext &)
    {
        auto &brightnessController = indicators::getMonitorBrightnessController();

        if (command == CMD_TOGGLE_DISPLAY)
        {
            brightnessController.toggleDisplayOffOn();
            return;
        }

        if (command == CMD_CYCLE_BRIGHTNESS)
        {
            brightnessController.cycleBrightness();
        }
    }
}
