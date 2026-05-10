#include "controlSystemHelpers.hpp"

#include "app/ActionUartDispatcher.hpp"

namespace controlSystem
{

const char *getCommandNameById(CommandId commandId)
{
    for (size_t i = 0; i < kSimpleCommandCount; ++i) {
        if (sSimpleCommands[i].commandId == commandId)
            return sSimpleCommands[i].pLogTag;
    }
    return "UNKNOWN";
}

} // namespace controlSystem
