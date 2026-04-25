#pragma once

#include "actionsResponse.hpp"
#include "relayController.hpp"
#include "rpiBootManager.hpp"
#include "serial.hpp"

namespace controlSystem
{
    class PowerStateTransitionHandler
    {
    public:
        PowerStateTransitionHandler(serialBus::Serial &rSerial,
                                    RelayController &rRelayController,
                                    RpiBootManager &rRpiBootManager);

        bool handle(const actions::ActionResponse &response);

    private:
        serialBus::Serial &mrSerial;
        RelayController &mrRelayController;
        RpiBootManager &mrRpiBootManager;

        static constexpr const char *mspTag = "PowerStateTransitionHandler";
    };
}
