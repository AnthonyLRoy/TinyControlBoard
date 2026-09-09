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
                                                             SystemState &rSystemState,
                                                             IActivityStatusSink *p_activitySink)
        : mr_serial(rSerial),
          mr_relayController(rRelayController),
          mr_rpiBootManager(rRpiBootManager),
          mr_systemState(rSystemState),
          mp_activitySink(p_activitySink)
    {
    }

    void PowerStateTransitionHandler::reportStatus(ControlBoardWorkingStatus status)
    {
        if (mp_activitySink)
            mp_activitySink->setActivityStatus(status);
        else
            indicators::getActivityStatusLed().sendStatus(status);
    }

    void PowerStateTransitionHandler::publishPowerState(ControlBoardPowerState state)
    {
        indicators::getPowerLed().setState(state);
        indicators::getMonitorBrightnessController().setState(state);
        // Store immediately (not just at the end of handle()) so BLE clients see
        // intermediate states like GOING_TO_SLEEP instead of jumping from ON to SLEEP.
        mr_systemState.powerState.store(state);
    }

    void PowerStateTransitionHandler::runRpiShutdownSequence()
    {
        publishPowerState(ControlBoardPowerState::GOING_TO_SLEEP);
        mr_serial.sendUartCommand("RPi_Shutdown", CMD_SYS_RPI_SHUTDOWN);
        mr_rpiBootManager.waitForRpiShutdown(board::timing::k_rpiShutdownTimeoutMs);
        mr_relayController.shutdownRpi();
        vTaskDelay(pdMS_TO_TICKS(board::timing::k_rpiShutdownSettleDelayMs));
    }

    ControlBoardPowerState PowerStateTransitionHandler::handle(const actions::IAction &action)
    {
        const auto currentState = indicators::getPowerLed().getState();
        const auto transition = evaluatePowerTransition(currentState, action.releaseTimeMillis);

        if (transition == PowerTransitionAction::PowerOn)
        {
            publishPowerState(ControlBoardPowerState::TURNING_ON);
            ESP_LOGI(k_logTag, "Initiating Power ON sequence");

            // Illuminate all eight boot diagnostic LEDs.
            indicators::getBootDiagnosticLeds().begin();

            mr_relayController.setRelayWithDelay(PIN_RELAY_VCC_3V3_POWER, true, board::timing::k_vcc3v3OnDelayMs);
            indicators::getBootDiagnosticLeds().stageSuccess(indicators::BootStage::Vcc3v3Relay);

            mr_relayController.setRelayWithDelay(PIN_RELAY_DAC_POWER, true, board::timing::k_powerSettleDelayMs);
            indicators::getBootDiagnosticLeds().stageSuccess(indicators::BootStage::DacRelay);

            mr_relayController.setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, true, board::timing::k_powerSettleDelayMs);
            indicators::getBootDiagnosticLeds().stageSuccess(indicators::BootStage::OutputStage);

            mr_relayController.setRelayWithDelay(PIN_RELAY_RPI_POWER, true, board::timing::k_rpiOnDelayMs);

            // Stage 4: wait for Raspberry Pi communication.
            const bool booted = mr_rpiBootManager.waitForRpiToBoot(board::timing::k_rpiBootTimeoutMs);
            if (booted)
            {
                indicators::getBootDiagnosticLeds().stageSuccess(indicators::BootStage::RpiComms);
                publishPowerState(ControlBoardPowerState::ON);
                indicators::getButtonStatusLed().sendStatus(ControlBoardWorkingStatus::SolidIdle);
                reportStatus(ControlBoardWorkingStatus::Active);
                return ControlBoardPowerState::ON;
            }

            indicators::getBootDiagnosticLeds().stageFailure(indicators::BootStage::RpiComms);
            publishPowerState(ControlBoardPowerState::SLEEP);
            reportStatus(ControlBoardWorkingStatus::sleeping);
            ESP_LOGW(k_logTag, "Power ON sequence aborted because no RPi heartbeat was received");
            return ControlBoardPowerState::SLEEP;
        }

        ESP_LOGI(k_logTag, "Release Time MS: %" PRIu16 "", action.releaseTimeMillis);

        if (transition == PowerTransitionAction::Sleep)
        {
            ESP_LOGI(k_logTag, "Initiating sleep sequence");
            runRpiShutdownSequence();
            mr_relayController.setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, false, 0);
            indicators::getSpiLedDriver().setAllLeds(false);
            indicators::getButtonStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);
            publishPowerState(ControlBoardPowerState::SLEEP);
            reportStatus(ControlBoardWorkingStatus::sleeping);
            return ControlBoardPowerState::SLEEP;
        }

        if (transition == PowerTransitionAction::DeepSleep)
        {
            ESP_LOGI(k_logTag, "Initiating deep sleep sequence");
            runRpiShutdownSequence();
            mr_relayController.setRelayWithDelay(PIN_RELAY_DAC_POWER, false, 0);
            mr_relayController.setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, false, 0);
            indicators::getSpiLedDriver().setAllLeds(false);
            indicators::getButtonStatusLed().sendStatus(ControlBoardWorkingStatus::Idle);
            publishPowerState(ControlBoardPowerState::DEEPSLEEP);
            reportStatus(ControlBoardWorkingStatus::sleeping);
            return ControlBoardPowerState::DEEPSLEEP;
        }

        // PowerTransitionAction::None — system in a transitional state; no change.
        return currentState;
    }
}