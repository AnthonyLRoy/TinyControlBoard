import re
import subprocess
import threading

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


ROTARY_DEBOUNCE_SECONDS = 1.0
ROTARY_WRAP_AROUND = True  # False clamps at first/last track instead
ROTARY_KEEP_PLAY_STATE = True  # False always starts playing after the jump

_rotary_lock = threading.Lock()
_rotary_pending = 0
_rotary_timer = None

_STATUS_RE = re.compile(r"\[(playing|paused)\]\s+#(\d+)/(\d+)")


def _get_mpc_state():
    """Return (state, position, length); position is 1-based, None if stopped."""
    result = subprocess.run(["mpc", "status"], capture_output=True, text=True, check=False)
    match = _STATUS_RE.search(result.stdout)
    if match:
        return match.group(1), int(match.group(2)), int(match.group(3))
    length_result = subprocess.run(["mpc", "playlist"], capture_output=True, text=True, check=False)
    length = len([line for line in length_result.stdout.splitlines() if line.strip()])
    return "stopped", None, length


def _rotary_timer_fired():
    global _rotary_pending, _rotary_timer
    with _rotary_lock:
        pending = _rotary_pending
        _rotary_pending = 0
        _rotary_timer = None
        if pending == 0:
            return
        state, position, length = _get_mpc_state()
        if length <= 0:
            return
        if position is None:
            position = 0 if pending > 0 else length + 1
        target = position + pending
        if ROTARY_WRAP_AROUND:
            target = (target - 1) % length + 1
        else:
            target = max(1, min(length, target))
        run_command(["mpc", "play", str(target)])
        if ROTARY_KEEP_PLAY_STATE and state == "paused":
            run_command(["mpc", "pause"])
        elif ROTARY_KEEP_PLAY_STATE and state == "stopped":
            run_command(["mpc", "stop"])


def handle_rotary_action(params):
    global _rotary_pending, _rotary_timer
    delta = 1 if params[0] == ROTARY_ACTION_NEXT else -1
    with _rotary_lock:
        _rotary_pending += delta
        if _rotary_timer is not None:
            _rotary_timer.cancel()
        _rotary_timer = threading.Timer(ROTARY_DEBOUNCE_SECONDS, _rotary_timer_fired)
        _rotary_timer.daemon = True
        _rotary_timer.start()


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
