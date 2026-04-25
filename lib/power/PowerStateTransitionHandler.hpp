#pragma once

#include "input/actions/actionsResponse.hpp"
#include "power/RelayController.hpp"
#include "power/RPIBootManager.hpp"
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