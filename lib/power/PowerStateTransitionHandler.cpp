#include "power/PowerStateTransitionHandler.hpp"

#include "board/boardConfig.hpp"
#include "indicators/ledManager.hpp"
#include "indicators/powerLed.hpp"
#include "power/PowerStateTransitionPolicy.hpp"
#include <inttypes.h>

namespace controlSystem
{

    PowerStateTransitionHandler::PowerStateTransitionHandler(transport::uart::UartTransport &rSerial,
                                                             RelayController &rRelayController,
                                                             RpiBootManager &rRpiBootManager,
                                                             IActivityStatusSink *p_activitySink)
        : mr_serial(rSerial),
          mr_relayController(rRelayController),
          mr_rpiBootManager(rRpiBootManager),
          mp_activitySink(p_activitySink)
    {
    }

    bool PowerStateTransitionHandler::isAwaitingLateBootHeartbeat() const
    {
        return m_delayedBootRecovery.isPending();
    }

    void PowerStateTransitionHandler::enterOnState()
    {
        setIndicatorState(ControlBoardPowerState::ON);
        indicators::getButtonStatusLed().sendStatus(ControlBoardWorkingStatus::SolidIdle);
        reportStatus(ControlBoardWorkingStatus::Active);
    }

    bool PowerStateTransitionHandler::completePendingBootOnHeartbeat()
    {
        const auto state = indicators::getPowerLed().getState();
        if (!m_delayedBootRecovery.consumeIfRecoverableState(state))
        {
            return false;
        }

        ESP_LOGI(k_logTag, "Delayed RPi heartbeat received; resuming deferred power-on completion");
        indicators::getSpiBootIndicator().notifySuccess();
        enterOnState();
        return true;
    }

    void PowerStateTransitionHandler::reportStatus(ControlBoardWorkingStatus status)
    {
        if (mp_activitySink)
            mp_activitySink->setActivityStatus(status);
        else
            indicators::getActivityStatusLed().sendStatus(status);
    }

    void PowerStateTransitionHandler::setIndicatorState(ControlBoardPowerState state)
    {
        indicators::getPowerLed().setState(state);
        indicators::getMonitorBrightnessController().setState(state);
    }

    void PowerStateTransitionHandler::runRpiShutdownSequence(ControlBoardPowerState postShutdownState)
    {
        m_delayedBootRecovery.clear();
        setIndicatorState(ControlBoardPowerState::SHUTTING_DOWN);
        mr_serial.sendUartCommand("RPi_Shutdown", CMD_SYS_RPI_SHUTDOWN);
        mr_rpiBootManager.waitForRpiShutdown(board::timing::k_rpiShutdownTimeoutMs);
        setIndicatorState(postShutdownState);
        mr_relayController.shutdownRpi();
        vTaskDelay(pdMS_TO_TICKS(board::timing::k_rpiShutdownSettleDelayMs));
        mr_relayController.shutdownScreen();
    }

    ControlBoardPowerState PowerStateTransitionHandler::handle(const actions::IAction &action)
    {
        const auto currentState = indicators::getPowerLed().getState();
        const auto transition = evaluatePowerTransition(currentState, action.releaseTimeMillis);

        if (transition == PowerTransitionAction::PowerOn)
        {
            m_delayedBootRecovery.clear();
            setIndicatorState(getTransitionEntryState(transition, currentState));
            ESP_LOGI(k_logTag, "Initiating Power ON sequence");

            mr_relayController.setRelayWithDelay(board::relays::k_screenPower, true, board::timing::k_screenOnDelayMs);
            mr_relayController.setRelayWithDelay(board::relays::k_dacPower, true, board::timing::k_powerSettleDelayMs);
            mr_relayController.setRelayWithDelay(board::relays::k_outputStagePower, true, board::timing::k_powerSettleDelayMs);
            mr_relayController.setRelayWithDelay(board::relays::k_rpiPower, true, board::timing::k_screenOnDelayMs);

            // Stage 4: wait for Raspberry Pi communication.
            const bool booted = mr_rpiBootManager.waitForRpiToBoot(board::timing::k_rpiBootTimeoutMs);
            
            if (booted)
            {
                indicators::getSpiBootIndicator().notifySuccess();
                enterOnState();
                return ControlBoardPowerState::ON;
            }

            indicators::getSpiBootIndicator().notifyFailure();
            m_delayedBootRecovery.markBootTimedOut();
            setIndicatorState(ControlBoardPowerState::SLEEP);
            reportStatus(ControlBoardWorkingStatus::sleeping);
            ESP_LOGW(k_logTag, "Power ON sequence entered degraded mode because no RPi heartbeat was received");
            return ControlBoardPowerState::SLEEP;
        }

        ESP_LOGI(k_logTag, "Release Time MS: %" PRIu16 "", action.releaseTimeMillis);

        if (transition == PowerTransitionAction::Sleep)
        {
            ESP_LOGI(k_logTag, "Initiating sleep sequence");
            runRpiShutdownSequence(getPostShutdownTransitionState(transition, currentState));
            vTaskDelay(pdMS_TO_TICKS(board::timing::k_screenPowerOffDelayMs));
            indicators::getSpiLedDriver().setAllLeds(false);
            indicators::getButtonStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);
            setIndicatorState(ControlBoardPowerState::SLEEP);
            reportStatus(ControlBoardWorkingStatus::sleeping);
            return ControlBoardPowerState::SLEEP;
        }

        if (transition == PowerTransitionAction::DeepSleep)
        {
            ESP_LOGI(k_logTag, "Initiating deep sleep sequence");
            runRpiShutdownSequence(getPostShutdownTransitionState(transition, currentState));
            mr_relayController.setRelayWithDelay(board::relays::k_dacPower, false, 0);
            mr_relayController.setRelayWithDelay(board::relays::k_outputStagePower, false, 0);
            indicators::getSpiLedDriver().setAllLeds(false);
            indicators::getButtonStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);
            setIndicatorState(ControlBoardPowerState::DEEPSLEEP);
            reportStatus(ControlBoardWorkingStatus::sleeping);
            return ControlBoardPowerState::DEEPSLEEP;
        }

        // PowerTransitionAction::None — system in a transitional state; no change.
        return currentState;
    }
}