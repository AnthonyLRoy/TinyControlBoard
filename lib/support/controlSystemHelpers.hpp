#pragma once

#include <cstdint>

#include "protocol/uartProtocol.hpp"

namespace controlSystem
{
    const char *getCommandNameById(CommandId commandId);
}
