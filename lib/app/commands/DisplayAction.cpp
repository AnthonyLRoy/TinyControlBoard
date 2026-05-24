#include "app/commands/DisplayAction.hpp"
#include "app/ActionContext.hpp"
#include "indicators/ledManager.hpp"

namespace actions
{
    void DisplayAction::execute(controlSystem::ActionContext &)
    {
        indicators::getMonitorBrightnessController().setBlanked(command == CMD_DISPLAY_OFF);
    }
}
