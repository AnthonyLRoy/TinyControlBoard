#pragma once

#include "input/actions/actionsResponse.hpp"
#include "power/RelayController.hpp"
#include "power/RPIBootManager.hpp"
#include "transport/uart/serial.hpp"

namespace controlSystem
{
    class PowerStateTransitionHandler
    {
    public:
        PowerStateTransitionHandler(transport::uart::UartTransport &rSerial,
                                    RelayController &rRelayController,
                                    RpiBootManager &rRpiBootManager);

        bool handle(const actions::ActionResponse &response);

    private:
        transport::uart::UartTransport &mrSerial;
        RelayController &mrRelayController;
        RpiBootManager &mrRpiBootManager;

        static constexpr const char *mspTag = "PowerStateTransitionHandler";
    };
}