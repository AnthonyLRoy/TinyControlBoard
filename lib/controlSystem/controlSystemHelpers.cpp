#include "controlSytemHelpers.hpp"
#include "actionProcessor.hpp"

#define TAG "CTRL_HELPERS"
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