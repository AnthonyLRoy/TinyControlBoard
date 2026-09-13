import subprocess

PARAM_DISABLED = 0
PARAM_ENABLED = 1
ROTARY_ACTION_PREVIOUS = 0
ROTARY_ACTION_NEXT = 1


def run_command(args):
    result = subprocess.run(args, check=False)
    if result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}: {args}", flush=True)


def set_meter_display(enabled):
    if enabled:
        run_command(["sudo", "moodeutl", "--setdisplay", "peppy"])
    else:
        run_command(["sudo", "moodeutl", "--setdisplay", "webui"])


def toggle_meter_display(params):
    set_meter_display(params[0] == PARAM_ENABLED)


def handle_rotary_action(params):
    if params[0] == ROTARY_ACTION_NEXT:
        run_command(["mpc", "next"])
    else:
        run_command(["mpc", "prev"])


def toggle_cover_view(params):
    if params[0] == PARAM_ENABLED:
        run_command(["/var/www/util/coverview.php", "-on"])
    else:
        run_command(["/var/www/util/coverview.php", "-off"])


def toggle_repeat(params):
    if params[0] == PARAM_ENABLED:
        run_command(["mpc", "repeat", "on"])
    else:
        run_command(["mpc", "repeat", "off"])


def toggle_random(params):
    if params[0] == PARAM_ENABLED:
        run_command(["mpc", "random", "on"])
    else:
        run_command(["mpc", "random", "off"])


def handle_rpi_shutdown(params):
    run_command(["sudo", "moodeutl", "--shutdown"])


def handle_next_track(params):
    run_command(["mpc", "next"])


def handle_previous_track(params):
    run_command(["mpc", "prev"])


def handle_play_pause(params):
    run_command(["mpc", "toggle"])


def handle_stop_track(params):
    run_command(["mpc", "stop"])


def handle_skip_forward(params):
    run_command(["mpc", "seek", "+10"])


def handle_skip_back(params):
    run_command(["mpc", "seek", "-10"])


def handle_seek_to_percent(params):
    percent = max(0, min(100, params[0]))
    run_command(["mpc", "seek", f"{percent}%"])
