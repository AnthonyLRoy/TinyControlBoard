#include "controlSystemHelpers.hpp"

#include "actionProcessor.hpp"

namespace controlSystem
{

const char *getCommandNameById(CommandId commandId)
{
    for (size_t i = 0; i < NUM_COMMANDS; ++i) {
        if (commandConfigs[i].commandId == commandId)
            return commandConfigs[i].pLogTag;
    }
    return "UNKNOWN";
}

} // namespace controlSystem
