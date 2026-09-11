import serial
import time
import RPi.GPIO as GPIO

import command_ids as cmd
import panel_control as panel
import playback_commands as playback
import library_browser as library
import playlist_manager as playlist
from protocol import compute_checksum, read_packet_with_resync

# === CONFIG ===
UART_PORT = "/dev/ttyAMA5"   # UART5
BAUD_RATE = 115200
DRDY_PIN = 23
DEFAULT_METER_ENABLED = False


def make_select_panel_handler(panel_index):
    panel_handler = panel.make_select_panel_handler(panel_index)

    def handler(params):
        library.set_radio_browse(panel_index == 1)
        panel_handler(params)

    return handler

# Commands whose payload is a raw UTF-8 name string (not 5×uint16 params).
TEXT_PAYLOAD_COMMANDS = {
    cmd.CMD_PLAYLIST_SAVE,
    cmd.CMD_PLAYLIST_SAVE_OVERWRITE,
    cmd.CMD_PLAYLIST_LOAD,
    cmd.CMD_PLAYLIST_DELETE,
    cmd.CMD_LIBRARY_SEARCH_ARTIST,
    cmd.CMD_LIBRARY_SEARCH_ALBUM,
    cmd.CMD_LIBRARY_SEARCH_ANY,
}

COMMAND_HANDLERS = {
    cmd.CMD_SYS_RPI_SHUTDOWN: playback.handle_rpi_shutdown,
    cmd.CMD_NEXT_TRACK: playback.handle_next_track,
    cmd.CMD_PREVIOUS_TRACK: playback.handle_previous_track,
    cmd.CMD_PLAY_PAUSE: playback.handle_play_pause,
    cmd.CMD_STOP_TRACK: playback.handle_stop_track,
    cmd.CMD_SKIP_FORWARD: playback.handle_skip_forward,
    cmd.CMD_SKIP_BACK: playback.handle_skip_back,
    cmd.CMD_PREV_MENU_ITEM: panel.handle_prev_panel,
    cmd.CMD_NEXT_MENU_ITEM: panel.handle_next_panel,
    cmd.CMD_ROTARY_ACTION: playback.handle_rotary_action,
    cmd.CMD_TOGGLE_METER: playback.toggle_meter_display,
    cmd.CMD_TOGGLE_COVER_VIEW: playback.toggle_cover_view,
    cmd.CMD_TOGGLE_REPEAT: playback.toggle_repeat,
    cmd.CMD_TOGGLE_RANDOM: playback.toggle_random,
    cmd.CMD_SELECT_PANEL_PLAYBACK: make_select_panel_handler(0),
    cmd.CMD_SELECT_PANEL_RADIO:    make_select_panel_handler(1),
    cmd.CMD_SELECT_PANEL_PLAYLIST: make_select_panel_handler(2),
    cmd.CMD_SELECT_PANEL_FOLDER:   make_select_panel_handler(3),
    cmd.CMD_SELECT_PANEL_TAG:      make_select_panel_handler(4),
    cmd.CMD_SELECT_PANEL_ALBUM:    make_select_panel_handler(5),
    cmd.CMD_BROWSE_REQUEST:        library.handle_browse_request,
    cmd.CMD_ADD_TRACK:             library.handle_add_track,
    cmd.CMD_PLAYLIST_REQUEST:      library.handle_playlist_request,
    cmd.CMD_PLAY_TRACK:            library.handle_play_track,
    cmd.CMD_REMOVE_TRACK:          library.handle_remove_track,
    cmd.CMD_ADD_FOLDER:            library.handle_add_folder,
    cmd.CMD_REPLACE_WITH_FOLDER:   library.handle_replace_with_folder,
    cmd.CMD_PLAYLIST_LIST_REQUEST: playlist.handle_playlist_list_request,
    cmd.CMD_PLAYLIST_SAVE:            playlist.handle_playlist_save,
    cmd.CMD_PLAYLIST_SAVE_OVERWRITE:  playlist.handle_playlist_save_overwrite,
    cmd.CMD_PLAYLIST_LOAD:            playlist.handle_playlist_load,
    cmd.CMD_PLAYLIST_DELETE:          playlist.handle_playlist_delete,
    cmd.CMD_CLEAR_QUEUE:              library.handle_clear_queue,
    cmd.CMD_LIBRARY_SEARCH_ARTIST:    library.handle_library_search_artist,
    cmd.CMD_LIBRARY_SEARCH_ALBUM:     library.handle_library_search_album,
    cmd.CMD_LIBRARY_SEARCH_ANY:       library.handle_library_search_any,
    cmd.CMD_ADD_SEARCH_RESULT:        library.handle_add_search_result,
}

def setup_gpio():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(DRDY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

def handle_command(command_id, params, payload):
    print(f"Handling Command ID: {command_id:#06x}, Params: {params}", flush=True)

    handler = COMMAND_HANDLERS.get(command_id)
    if handler is None:
        print("⚠️ Unknown command", flush=True)
        return

    if command_id in TEXT_PAYLOAD_COMMANDS:
        handler(payload.decode("utf-8", errors="replace"))
    else:
        handler(params)

def read_and_process_packet(ser):
    data = read_packet_with_resync(ser)

    src_app = data[2]
    if src_app != 0x01:  # APP_ESP32; drop looped-back RPi transmissions
        print(f"⚠️ Dropping packet from src_app=0x{src_app:02X} (expected ESP32=0x01)", flush=True)
        return

    payload_len = data[7]
    checksum = data[8 + payload_len]

    if checksum != compute_checksum(data):
        print("⚠️ Checksum mismatch", flush=True)
        return

    cmd_id  = data[5] | (data[6] << 8)
    payload = data[8:8 + payload_len]

    # Decode params as 5×LE uint16 for command packets with ≥10 payload bytes
    params = [0, 0, 0, 0, 0]
    for i in range(min(5, payload_len // 2)):
        params[i] = payload[i * 2] | (payload[i * 2 + 1] << 8)

    handle_command(cmd_id, params, payload)

def wait_for_data_ready():
    result = GPIO.wait_for_edge(DRDY_PIN, GPIO.RISING, timeout=10000)
    if result is None:
        print(f"⚠️ No DRDY pulse on GPIO {DRDY_PIN} in 10 s — ESP32 not sending", flush=True)

def main():
    ser = None

    try:
        setup_gpio()
        ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=0.01)
        playback.set_meter_display(DEFAULT_METER_ENABLED)
        print("🎧 UART5 listener started", flush=True)

        while True:
            read_and_process_packet(ser)
            time.sleep(0.001)
    except KeyboardInterrupt:
        print("Exiting...", flush=True)
    finally:
        if ser is not None and ser.is_open:
            ser.close()
        GPIO.cleanup()

if __name__ == "__main__":    
    main()
