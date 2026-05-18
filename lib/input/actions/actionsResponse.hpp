#pragma once

#include "protocol/uartProtocol.hpp"

namespace actions
{
    struct ActionResponse
    {
        bool isActive = false;
        CommandId command = CMD_NO_ACTION;
        uint16_t parameters[5]{0, 0, 0, 0, 0};
        uint16_t releaseTimeMillis = 0;

        ActionResponse() = default;
    };
}