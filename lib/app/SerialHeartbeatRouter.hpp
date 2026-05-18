#pragma once

#include "protocol/uartProtocol.hpp"
#include <cstdint>

namespace controlSystem
{
    struct IHeartbeatSink
    {
        virtual ~IHeartbeatSink() = default;
        virtual void handleHeartbeatReceived() = 0;
    };

    class SerialHeartbeatRouter
    {
    public:
        static constexpr uint16_t k_legacyHeartbeatCommandId = 0x9999;

        explicit SerialHeartbeatRouter(IHeartbeatSink *p_heartbeatSink)
            : mp_heartbeatSink(p_heartbeatSink)
        {
        }

        bool route(const UartMessage &message) const
        {
            if (message.commandId != CMD_SYS_HEARTBEAT &&
                message.commandId != k_legacyHeartbeatCommandId)
            {
                return false;
            }

            if (mp_heartbeatSink)
            {
                mp_heartbeatSink->handleHeartbeatReceived();
            }
            return true;
        }

    private:
        IHeartbeatSink *mp_heartbeatSink;
    };
}