#include "app/actionProcessor.hpp"
#include "app/ActionFactory.hpp"
#include "app/ControlBoardButtonIds.hpp"
#include "board/boardConfig.hpp"
#include "indicators/ledManager.hpp"
#include <inttypes.h>
#include <atomic>

namespace controlSystem
{
    namespace
    {
        bool getToggleButtonId(CommandId command, uint8_t &buttonId)
        {
            switch (command)
            {
            case CMD_TOGGLE_DISPLAY:
                buttonId = controlBoardButtons::k_toggleDisplay;
                return true;
            case CMD_TOGGLE_COVER_VIEW:
                buttonId = controlBoardButtons::k_cover;
                return true;
            case CMD_TOGGLE_REPEAT:
                buttonId = controlBoardButtons::k_repeat;
                return true;
            case CMD_TOGGLE_RANDOM:
                buttonId = controlBoardButtons::k_toggleRandom;
                return true;
            case CMD_TOGGLE_DAC:
                buttonId = controlBoardButtons::k_toggleDac;
                return true;
            case CMD_TOGGLE_METER:
                buttonId = controlBoardButtons::k_toggleMeter;
                return true;
            default:
                return false;
            }
        }
    }

    ActionProcessor::ActionProcessor(transport::uart::UartTransport &rSerialBus, relays::StandardRelay &rRelays, SystemState &rSystemState, IActivityStatusSink *p_activitySink, std::function<void(uint8_t)> onRemoteToggle)
        : mr_serial(rSerialBus), mr_relays(rRelays), mr_systemState(rSystemState), m_onRemoteToggle(std::move(onRemoteToggle))
    {
        mp_serialUartCommandSink = std::make_unique<SerialUartCommandSink>(mr_serial);
        mp_actionUartDispatcher = std::make_unique<ActionUartDispatcher>(*mp_serialUartCommandSink);
        mp_rpiBootManager = std::make_unique<RpiBootManager>();
        mp_relayController = std::make_unique<RelayController>(mr_serial, mr_relays);
        mp_powerStateTransitionHandler = std::make_unique<PowerStateTransitionHandler>(
            mr_serial,
            *mp_relayController,
            *mp_rpiBootManager,
            mr_systemState,
            p_activitySink);
    }

    void ActionProcessor::process(std::unique_ptr<actions::IAction> iaction, bool isRemoteOrigin)
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

        uint8_t buttonId = 0;
        if (isRemoteOrigin && m_onRemoteToggle && getToggleButtonId(iaction->command, buttonId))
        {
            m_onRemoteToggle(buttonId);
        }
    }

    void ActionProcessor::injectCommand(CommandId cmd, uint16_t releaseMs)
    {
        // Best-effort injection from external tasks (e.g. BLE). Uses an atomic
        // flag to prevent concurrent execution without blocking long operations.
        static std::atomic_bool s_busy{false};
        if (s_busy.exchange(true, std::memory_order_acquire))
        {
            ESP_LOGW(k_logTag, "injectCommand: processor busy, dropping cmd 0x%04X", cmd);
            return;
        }
        auto action = createAction(cmd, releaseMs);
        if (action)
        {
            process(std::move(action), /*isRemoteOrigin=*/true);
        }
        s_busy.store(false, std::memory_order_release);
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

    void ActionProcessor::resetToggleStates()
    {
        if (mp_actionUartDispatcher)
        {
            mp_actionUartDispatcher->resetToggleStates();
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