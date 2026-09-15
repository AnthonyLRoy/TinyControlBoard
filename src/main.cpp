#include "main.h"
#include "input/input.hpp"
#include "ble/BleServer.hpp"
#include "hal/uart/serial.hpp"
#include "protocol/uartProtocol.hpp"
#include "indicators/ledManager.hpp"
#include "esp_pm.h"

#define DELAY_STARTUP_TIME_MS 5000
#define MAX_CLOCK_FREQ_MHZ 240
#define MIN_CLOCK_FREQ_MHZ 40

extern "C" void app_main(void)
{

    // does not work :(  does not save any power at all 
    esp_pm_config_t pmConfig = {
        .max_freq_mhz = MAX_CLOCK_FREQ_MHZ,
        .min_freq_mhz = MIN_CLOCK_FREQ_MHZ,
        .light_sleep_enable = true
    };
    esp_pm_configure(&pmConfig);

    vTaskDelay(pdMS_TO_TICKS(DELAY_STARTUP_TIME_MS));  //this is really unfortunate , but we need to wait for the power to stabilise before we start doing anything

    controlSystem::ControlBoard board;
    if (!board.init())
    {
        // Single attempt only: retrying risks re-driving relays/power sequencing into a bad config.
        ESP_LOGE("main", "ControlBoard init failed; stopping");
        board.deinit();
        indicators::getBootDiagnosticLeds().begin();
        indicators::getBootDiagnosticLeds().firmwareInitFailed();
        return;
    }

    static ble::BleServer bleServer;
    bleServer.start(board.getActionProcessor(), board.getSystemState(),
                    [](uint16_t cmdId, uint16_t param)
                    {
                        UartMessage msg;
                        msg.msgType   = MSG_COMMAND;
                        msg.commandId = cmdId;
                        msg.params[0] = param;
                        transport::uart::UartTransport::getInstance().sendUartMessage("BLE_Library", msg);
                    },
                    [](uint16_t cmdId, const uint8_t *p_name, uint8_t nameLen)
                    {
                        UartMessage msg;
                        msg.msgType   = MSG_PLAYLIST_CMD;
                        msg.commandId = cmdId;
                        msg.playlistNameOutLen = nameLen;
                        memcpy(msg.playlistNameOut, p_name, nameLen);
                        transport::uart::UartTransport::getInstance().sendUartMessage("BLE_Playlist", msg);
                    });
    board.setLibraryEntryCallback([](const UartMessage &m) { ble::notifyLibraryEntry(m); });
    board.setPlaylistResultCallback([](const UartMessage &m) { ble::notifyPlaylistResult(m); });

    // Keep  alive; all work is done in tasks .
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
