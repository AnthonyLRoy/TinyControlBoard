#include "controlSytemHelpers.hpp"
#include "actionProcessor.hpp"

namespace controlSystem
{

const char* getCommandNameForPin(uint8_t pin)
{

    size_t index = pin - 1;

    if (index >= NUM_COMMANDS)
        return "UNKNOWN";

    return commandConfigs[index].logTag;   // or .label or .id — whichever is correct
}

} // namespace controlSystem