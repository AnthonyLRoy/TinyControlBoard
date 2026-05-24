#pragma once

#include "protocol/uartProtocol.hpp"
#include <cstdint>

namespace controlSystem
{
    inline constexpr uint16_t kLegacyHeartbeatCommandId = 0x9999;

    constexpr bool isHeartbeatCommand(uint16_t commandId)
    {
        return commandId == CMD_SYS_HEARTBEAT || commandId == kLegacyHeartbeatCommandId;
    }
}