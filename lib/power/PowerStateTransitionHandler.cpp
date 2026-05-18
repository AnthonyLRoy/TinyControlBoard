#include "power/PowerStateTransitionHandler.hpp"

#include "ledManager.hpp"
#include "powerLed.hpp"
#include "power/PowerStateTransitionPolicy.hpp"
#include <inttypes.h>

namespace controlSystem
{
    namespace
    {
        constexpr uint32_t POWER_SETTLE_DELAY_MS = 1500;
        constexpr uint32_t SCREEN_ON_DELAY_MS = 1000;
        constexpr uint32_t RPI_BOOT_TIMEOUT_MS = 60000;
        constexpr uint32_t RPI_SHUTDOWN_TIMEOUT_MS = 60000;
        constexpr uint32_t RPI_SHUTDOWN_SETTLE_DELAY_MS = 500;
        constexpr uint32_t SCREEN_POWER_OFF_DELAY_MS = 5000;
    }

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

    bool PowerStateTransitionHandler::handle(const actions::ActionResponse &response)
    {
        const auto powerState = indicators::getPowerLed().getState();
        const auto transitionAction = PowerStateTransitionPolicy::evaluate(powerState, response.releaseTimeMillis);

        if (transitionAction == PowerTransitionAction::PowerOn)
        {
            indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);
            indicators::getMonitorBrightnessController().setState(ControlBoardPowerState::TURNING_ON);
            ESP_LOGI(k_logTag, "Initiating Power ON sequence");

            mr_relayController.setRelayWithDelay(PIN_RELAY_SCREEN_POWER, true, SCREEN_ON_DELAY_MS);
            mr_relayController.setRelayWithDelay(PIN_RELAY_DAC_POWER, true, POWER_SETTLE_DELAY_MS);
            mr_relayController.setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, true, POWER_SETTLE_DELAY_MS);
            mr_relayController.setRelayWithDelay(PIN_RELAY_RPI_POWER, true, SCREEN_ON_DELAY_MS);

            indicators::getSpiBootIndicator().startWaiting();
            const bool booted = mr_rpiBootManager.waitForRpiToBoot(RPI_BOOT_TIMEOUT_MS);
            if (booted)
            {
                indicators::getSpiBootIndicator().notifySuccess();
                indicators::getPowerLed().setState(ControlBoardPowerState::ON);
                indicators::getMonitorBrightnessController().setState(ControlBoardPowerState::ON);
                if (mp_activitySink)
                    mp_activitySink->setActivityStatus(ControlBoardWorkingStatus::Active);
                else
                    indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Active);
                return true;
            }

            indicators::getSpiBootIndicator().notifyFailure();
            indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);
            indicators::getMonitorBrightnessController().setState(ControlBoardPowerState::SLEEP);
            if (mp_activitySink)
                mp_activitySink->setActivityStatus(ControlBoardWorkingStatus::sleeping);
            else
                indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::sleeping);
            ESP_LOGW(k_logTag, "Power ON sequence aborted because no RPi heartbeat was received");
            return false;
        }

        ESP_LOGI(k_logTag, "Release Time MS: %" PRIu16 "", response.releaseTimeMillis);
        if (transitionAction == PowerTransitionAction::Sleep)
        {
            ESP_LOGI(k_logTag, "Initiating sleep sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);
            indicators::getMonitorBrightnessController().setState(ControlBoardPowerState::GOING_TO_SLEEP);

            mr_serial.sendUartCommand("RPi_Shutdown", CMD_SYS_RPI_SHUTDOWN);
            mr_rpiBootManager.waitForRpiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            mr_relayController.shutdownRpi(true);
            vTaskDelay(pdMS_TO_TICKS(RPI_SHUTDOWN_SETTLE_DELAY_MS));
            mr_relayController.shutdownScreen(false);
            vTaskDelay(pdMS_TO_TICKS(SCREEN_POWER_OFF_DELAY_MS));
            indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);
            indicators::getMonitorBrightnessController().setState(ControlBoardPowerState::SLEEP);
            if (mp_activitySink)
                mp_activitySink->setActivityStatus(ControlBoardWorkingStatus::sleeping);
            else
                indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::sleeping);
            return true;
        }

        if (transitionAction == PowerTransitionAction::DeepSleep)
        {
            ESP_LOGI(k_logTag, "Initiating deep sleep sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);
            indicators::getMonitorBrightnessController().setState(ControlBoardPowerState::GOING_TO_SLEEP);
            mr_serial.sendUartCommand("RPi_Shutdown", CMD_SYS_RPI_SHUTDOWN);
            mr_rpiBootManager.waitForRpiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            mr_relayController.shutdownRpi(true);
            vTaskDelay(pdMS_TO_TICKS(RPI_SHUTDOWN_SETTLE_DELAY_MS));
            mr_relayController.shutdownScreen(false);
            mr_relayController.setRelayWithDelay(PIN_RELAY_DAC_POWER, false, 0);
            mr_relayController.setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, false, 0);
            indicators::getPowerLed().setState(ControlBoardPowerState::DEEPSLEEP);
            indicators::getMonitorBrightnessController().setState(ControlBoardPowerState::DEEPSLEEP);
            if (mp_activitySink)
                mp_activitySink->setActivityStatus(ControlBoardWorkingStatus::sleeping);
            else
                indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::sleeping);
        }

        return true;
    }
}