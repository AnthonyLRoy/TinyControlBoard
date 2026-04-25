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
        static constexpr uint16_t kLegacyHeartbeatCommandId = 0x9999;

        explicit SerialHeartbeatRouter(IHeartbeatSink *pHeartbeatSink)
            : mpHeartbeatSink(pHeartbeatSink)
        {
        }

        bool route(const UartMessage &message) const
        {
            if (message.commandId != CMD_SYS_HEARTBEAT &&
                message.commandId != kLegacyHeartbeatCommandId)
            {
                return false;
            }

            if (mpHeartbeatSink)
            {
                mpHeartbeatSink->handleHeartbeatReceived();
            }
            return true;
        }

    private:
        IHeartbeatSink *mpHeartbeatSink;
    };
}