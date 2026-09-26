import subprocess

PARAM_DISABLED = 0
PARAM_ENABLED = 1
ROTARY_ACTION_PREVIOUS = 0
ROTARY_ACTION_NEXT = 1


def run_command(args):
    """Runs a system command and reports nonzero exit status."""
    result = subprocess.run(args, check=False)
    if result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}: {args}", flush=True)


def set_meter_display(enabled):
    """Selects the moOde meter visualization or its standard web interface."""
    if enabled:
        run_command(["sudo", "moodeutl", "--setdisplay", "peppy"])
    else:
        run_command(["sudo", "moodeutl", "--setdisplay", "webui"])


def toggle_meter_display(params):
    """Applies the meter-display state encoded in the command parameters."""
    set_meter_display(params[0] == PARAM_ENABLED)


def handle_rotary_action(params):
    """Moves to the previous or next track based on the rotary direction."""
    if params[0] == ROTARY_ACTION_NEXT:
        run_command(["mpc", "next"])
    else:
        run_command(["mpc", "prev"])


def toggle_cover_view(params):
    """Enables or disables the moOde cover-view display mode."""
    if params[0] == PARAM_ENABLED:
        run_command(["/var/www/util/coverview.php", "-on"])
    else:
        run_command(["/var/www/util/coverview.php", "-off"])


def toggle_repeat(params):
    """Enables or disables MPD repeat mode from the command parameter."""
    if params[0] == PARAM_ENABLED:
        run_command(["mpc", "repeat", "on"])
    else:
        run_command(["mpc", "repeat", "off"])


def toggle_random(params):
    """Enables or disables MPD random playback from the command parameter."""
    if params[0] == PARAM_ENABLED:
        run_command(["mpc", "random", "on"])
    else:
        run_command(["mpc", "random", "off"])


def handle_rpi_shutdown(params):
    """Requests a graceful moOde system shutdown."""
    run_command(["sudo", "moodeutl", "--shutdown"])


def handle_next_track(params):
    """Skips playback to the next MPD queue entry."""
    run_command(["mpc", "next"])


def handle_previous_track(params):
    """Skips playback to the previous MPD queue entry."""
    run_command(["mpc", "prev"])


def handle_play_pause(params):
    """Toggles MPD playback between playing and paused states."""
    run_command(["mpc", "toggle"])


def handle_stop_track(params):
    """Stops MPD playback."""
    run_command(["mpc", "stop"])


def handle_skip_forward(params):
    """Seeks ten seconds forward in the current track."""
    run_command(["mpc", "seek", "+10"])


def handle_skip_back(params):
    """Seeks ten seconds backward in the current track."""
    run_command(["mpc", "seek", "-10"])


def handle_seek_to_percent(params):
    """Seeks to a clamped percentage of the current track's duration."""
    percent = max(0, min(100, params[0]))
    run_command(["mpc", "seek", f"{percent}%"])
