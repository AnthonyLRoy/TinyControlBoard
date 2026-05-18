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
                                    IActivityStatusSink *p_activitySink = nullptr);

        bool handle(const actions::ActionResponse &response);

    private:
        transport::uart::UartTransport &mr_serial;
        RelayController &mr_relayController;
        RpiBootManager &mr_rpiBootManager;
        IActivityStatusSink *mp_activitySink;

        static constexpr const char *k_logTag = "Power_State_Hdlr";
    };
}