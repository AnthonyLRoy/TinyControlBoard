#include "power/PowerStateTransitionHandler.hpp"

#include "led_manager.hpp"
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
    }

    PowerStateTransitionHandler::PowerStateTransitionHandler(transport::uart::UartTransport &rSerial,
                                                             RelayController &rRelayController,
                                                             RpiBootManager &rRpiBootManager)
        : mrSerial(rSerial),
          mrRelayController(rRelayController),
          mrRpiBootManager(rRpiBootManager)
    {
    }

    bool PowerStateTransitionHandler::handle(const actions::ActionResponse &response)
    {
        const auto powerState = indicators::getPowerLed().getState();
        const auto transitionAction = PowerStateTransitionPolicy::evaluate(powerState, response.releaseTimeMillis);

        if (transitionAction == PowerTransitionAction::PowerOn)
        {
            indicators::getPowerLed().setState(ControlBoardPowerState::TURNING_ON);
            ESP_LOGI(mspTag, "Initiating Power ON sequence");

            mrRelayController.setRelayWithDelay(PIN_RELAY_SCREEN_POWER, true, SCREEN_ON_DELAY_MS);
            mrRelayController.setRelayWithDelay(PIN_RELAY_DAC_POWER, true, POWER_SETTLE_DELAY_MS);
            mrRelayController.setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, true, POWER_SETTLE_DELAY_MS);
            mrRelayController.setRelayWithDelay(PIN_RELAY_RPI_POWER, true, SCREEN_ON_DELAY_MS);

            const bool booted = mrRpiBootManager.waitForRpiToBoot(RPI_BOOT_TIMEOUT_MS);
            indicators::getPowerLed().setState(ControlBoardPowerState::ON);
            indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::Active);
            return booted;
        }

        ESP_LOGI(mspTag, "Release Time MS: %" PRIu16 "", response.releaseTimeMillis);
        if (transitionAction == PowerTransitionAction::Sleep)
        {
            ESP_LOGI(mspTag, "Initiating Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);

            mrSerial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            mrRpiBootManager.waitForRpiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            mrRelayController.shutdownRpi(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            mrRelayController.shutdownScreen(false);
            vTaskDelay(pdMS_TO_TICKS(5000));
            indicators::getPowerLed().setState(ControlBoardPowerState::SLEEP);
            indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::sleeping);
            return true;
        }

        if (transitionAction == PowerTransitionAction::DeepSleep)
        {
            ESP_LOGI(mspTag, "Initiating Deep Sleep Sequence");
            indicators::getPowerLed().setState(ControlBoardPowerState::GOING_TO_SLEEP);
            mrSerial.sendUartCommand("RPISHUTDOWN", CMD_SYS_RPI_SHUTDOWN);
            mrRpiBootManager.waitForRpiShutdown(RPI_SHUTDOWN_TIMEOUT_MS);
            mrRelayController.shutdownRpi(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            mrRelayController.shutdownScreen(false);
            mrRelayController.setRelayWithDelay(PIN_RELAY_DAC_POWER, false, 0);
            mrRelayController.setRelayWithDelay(PIN_RELAY_OUTPUT_STAGE_POWER, false, 0);
            indicators::getPowerLed().setState(ControlBoardPowerState::DEEPSLEEP);
            indicators::getActivityStatusLed().sendStatus(ControlBoardWorkingStatus::sleeping);
        }

        return true;
    }
}