#pragma once

#include "input/actions/actionsResponse.hpp"
#include "power/RelayController.hpp"
#include "power/RPIBootManager.hpp"
#include "transport/uart/serial.hpp"
#include "activityStatus.hpp"

namespace controlSystem
{
    class PowerStateTransitionHandler
    {
    public:
        PowerStateTransitionHandler(transport::uart::UartTransport &rSerial,
                                    RelayController &rRelayController,
                                    RpiBootManager &rRpiBootManager,
                                    IActivityStatusSink *pActivitySink = nullptr);

        bool handle(const actions::ActionResponse &response);

    private:
        transport::uart::UartTransport &mrSerial;
        RelayController &mrRelayController;
        RpiBootManager &mrRpiBootManager;
        IActivityStatusSink *mpActivitySink;

        static constexpr const char *mspTag = "Power_State_Hdlr";
    };
}