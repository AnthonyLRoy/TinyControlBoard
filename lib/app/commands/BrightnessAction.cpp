#include "app/commands/BrightnessAction.hpp"
#include "app/ActionContext.hpp"
#include "indicators/ledManager.hpp"

namespace actions
{
    void BrightnessAction::execute(controlSystem::ActionContext &)
    {
        indicators::getMonitorBrightnessController().cycleBrightness();
    }
}
