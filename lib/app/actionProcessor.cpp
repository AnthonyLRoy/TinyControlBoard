#include "app/actionProcessor.hpp"
#include "indicators/ledManager.hpp"
#include <inttypes.h>

namespace controlSystem
{
    ActionProcessor::ActionProcessor(transport::uart::UartTransport &rSerialBus, relays::StandardRelay &rRelays, SystemState &rSystemState, IActivityStatusSink *p_activitySink)
        : mr_serial(rSerialBus), mr_relays(rRelays), mr_systemState(rSystemState)
    {
        mp_serialUartCommandSink = std::make_unique<SerialUartCommandSink>(mr_serial);
        mp_actionUartDispatcher = std::make_unique<ActionUartDispatcher>(*mp_serialUartCommandSink);
        mp_rpiBootManager = std::make_unique<RpiBootManager>();
        mp_relayController = std::make_unique<RelayController>(mr_serial, mr_relays);
        mp_powerStateTransitionHandler = std::make_unique<PowerStateTransitionHandler>(
            mr_serial,
            *mp_relayController,
            *mp_rpiBootManager,
            p_activitySink);
    }

    void ActionProcessor::process(const actions::Action &action)
    {
        ESP_LOGI(k_logTag, "Action processor received command: 0x%04X", action.command);

        auto iaction = createAction(action.command, action.releaseTimeMillis);
        if (!iaction)
            return;

        std::copy(std::begin(action.parameters), std::end(action.parameters),
                  std::begin(iaction->parameters));

        const auto powerState = mr_systemState.powerState.load();
        if (iaction->requiresPowerOn() && powerState != ControlBoardPowerState::ON)
        {
            ESP_LOGI(k_logTag, "Ignoring command %u as system is not ON", action.command);
            return;
        }

        ActionContext ctx{
            *mp_actionUartDispatcher,
            *mp_powerStateTransitionHandler,
            *mp_relayController,
            mr_serial,
            mr_systemState
        };

        iaction->execute(ctx);
    }

    bool ActionProcessor::handleInboundUartMessage(const UartMessage &message)
    {
        // Scaffold only: protocol-specific inbound handling will be added in a
        // dedicated pass once ACK/STATUS semantics are finalized.
        ESP_LOGI(k_logTag, "Inbound UART message received (type=%u cmd=0x%04X seq=%u)",
                 message.msgType, message.commandId, message.sequence);
        return false;
    }

    void ActionProcessor::handleHeartbeatReceived()
    {
        if (mp_rpiBootManager)
        {
            mp_rpiBootManager->handleHeartbeatReceived();
        }
    }

    void ActionProcessor::handleHeartbeatTimeout()
    {
        if (mp_rpiBootManager)
        {
            mp_rpiBootManager->handleHeartbeatTimeout();
        }
    }

    bool ActionProcessor::waitForRpiToBoot(uint32_t timeoutMs)
    {
        if (mp_rpiBootManager)
        {
            return mp_rpiBootManager->waitForRpiToBoot(timeoutMs);
        }
        return false;
    }

    bool ActionProcessor::waitForRpiShutdown(uint32_t timeoutMs)
    {
        if (mp_rpiBootManager)
        {
            return mp_rpiBootManager->waitForRpiShutdown(timeoutMs);
        }
        return false;
    }

    bool ActionProcessor::triggerInitialPowerOn()
    {
        // Force LED to OFF so PowerStateTransitionPolicy treats this as a power-on request.
        indicators::getPowerLed().setState(ControlBoardPowerState::OFF);
        actions::Action syntheticAction;
        syntheticAction.command = CMD_SYS_POWER;
        syntheticAction.releaseTimeMillis = 0;
        const bool result = mp_powerStateTransitionHandler->handle(syntheticAction);
        mr_systemState.powerState.store(indicators::getPowerLed().getState());
        return result;
    }
}