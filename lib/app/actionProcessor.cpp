#include "app/actionProcessor.hpp"
#include "app/ActionFactory.hpp"
#include "board/boardConfig.hpp"
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

    void ActionProcessor::process(std::unique_ptr<actions::IAction> iaction)
    {
        if (!iaction)
            return;

        ESP_LOGI(k_logTag, "Action processor received command: 0x%04X", iaction->command);

        const auto powerState = mr_systemState.powerState.load();
        if (iaction->requiresPowerOn() && powerState != ControlBoardPowerState::ON)
        {
            ESP_LOGI(k_logTag, "Ignoring command %u as system is not ON", iaction->command);
            return;
        }

        ActionContext ctx{
            *mp_actionUartDispatcher,
            *mp_powerStateTransitionHandler,
            *mp_relayController,
            mr_serial,
            mr_systemState,
            indicators::getPowerLed(),
            indicators::getMonitorBrightnessController()
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

        if constexpr (board::debug::k_simulateRpiBoot)
        {
            // In debug/simulate mode no real RPi is connected, so heartbeat timeouts
            // are expected and must not force the power state down.
            return;
        }

        const auto state = mr_systemState.powerState.load();
        if (state == ControlBoardPowerState::ON || state == ControlBoardPowerState::TURNING_ON)
        {
            ESP_LOGW(k_logTag, "Heartbeat lost while system was ON -- forcing power state to SLEEP");
            mr_systemState.powerState.store(ControlBoardPowerState::SLEEP);
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
        auto syntheticAction = createAction(CMD_SYS_POWER);
        const auto newState = mp_powerStateTransitionHandler->handle(*syntheticAction);
        mr_systemState.powerState.store(newState);
        return newState == ControlBoardPowerState::ON;
    }
}