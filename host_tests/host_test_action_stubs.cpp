// Stub execute() implementations for the host test build.
// The real implementations in lib/app/commands/*.cpp include ESP-IDF headers
// which cannot be compiled for the host (Windows/MSVC) target.
// These stubs satisfy the linker so ActionFactory can be linked and tested.
#include "app/commands/UartDispatchAction.hpp"
#include "app/commands/RelayAction.hpp"
#include "app/commands/PowerTransitionAction.hpp"
#include "app/commands/SystemAction.hpp"
#include "app/commands/DisplayAction.hpp"
#include "app/commands/BrightnessAction.hpp"

namespace actions
{
    void UartDispatchAction::execute(controlSystem::ActionContext &) {}
    void RelayAction::execute(controlSystem::ActionContext &) {}
    void PowerTransitionAction::execute(controlSystem::ActionContext &) {}
    void SystemAction::execute(controlSystem::ActionContext &) {}
    void DisplayAction::execute(controlSystem::ActionContext &) {}
    void BrightnessAction::execute(controlSystem::ActionContext &) {}
}
