#pragma once

#include "protocol/uartProtocol.hpp"

namespace controlSystem
{
    const char *getCommandNameById(CommandId commandId);
    const char *getSimpleCommandLogTag(CommandId commandId);
}
