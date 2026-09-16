#!/usr/bin/env bash
# Read-only installation and runtime checks for TinyControlBoard on moOde.

set -u

PASS_COUNT=0
WARN_COUNT=0
FAIL_COUNT=0

pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    printf '[PASS] %s\n' "$1"
}

warn() {
    WARN_COUNT=$((WARN_COUNT + 1))
    printf '[WARN] %s\n' "$1"
}

fail() {
    FAIL_COUNT=$((FAIL_COUNT + 1))
    printf '[FAIL] %s\n' "$1"
}

check_file() {
    local path="$1"
    local description="$2"
    if [[ -f "$path" ]]; then
        pass "$description: $path"
    else
        fail "$description is missing: $path"
    fi
}

check_command() {
    local command_name="$1"
    if command -v "$command_name" >/dev/null 2>&1; then
        pass "Command available: $command_name"
    else
        fail "Command is missing: $command_name"
    fi
}

echo "TinyControlBoard installation check"
echo "===================================="
echo "User: $(id -un)"
echo "Home: $HOME"
echo

echo "1. Repository and deployed files"
REPO_ROOT=""
if [[ -f "$(dirname "$0")/home/antho/uart5_listener.py" ]]; then
    REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
elif [[ -f "$HOME/TinyControlBoard/scripts/rpi/home/antho/uart5_listener.py" ]]; then
    REPO_ROOT="$HOME/TinyControlBoard"
fi

if [[ -n "$REPO_ROOT" ]]; then
    pass "Repository layout found: $REPO_ROOT"
else
    warn "Repository layout was not found relative to this script or $HOME/TinyControlBoard"
fi

DEPLOYED_FILES=(
    uart5_listener.py
    heartbeat_sender.py
    command_ids.py
    protocol.py
    uart_writer_client.py
    mpd_client.py
    library_browser.py
    playlist_manager.py
    panel_control.py
    playback_commands.py
)
for filename in "${DEPLOYED_FILES[@]}"; do
    check_file "$HOME/$filename" "Deployed file"
done

if [[ -n "$REPO_ROOT" ]]; then
    check_file "$REPO_ROOT/scripts/rpi/home/antho/UAart5Listener.py" "Legacy wrapper in repository"
    check_file "$REPO_ROOT/scripts/rpi/FinalSplashScreen.png" "Static splash image in repository"
    if [[ -d "$REPO_ROOT/scripts/rpi/Peppymeter/1024x600" ]]; then
        pass "PeppyMeter assets found in repository"
    else
        fail "PeppyMeter assets are missing from the repository"
    fi
fi

echo
echo "2. Required commands and Python modules"
for command_name in python3 systemctl ss; do
    check_command "$command_name"
done

if python3 -c 'import serial, RPi.GPIO, requests' >/dev/null 2>&1; then
    pass "Python modules available: pyserial, RPi.GPIO, requests"
else
    fail "One or more Python modules are unavailable: pyserial, RPi.GPIO, requests"
fi

echo
echo "3. Firmware and UART configuration"
CONFIG_FILE="/boot/firmware/config.txt"
USER_CONFIG_FILE="/boot/firmware/config-user.txt"
CMDLINE_FILE="/boot/firmware/cmdline.txt"

check_file "$CONFIG_FILE" "Firmware configuration"
check_file "$USER_CONFIG_FILE" "User firmware configuration"
check_file "$CMDLINE_FILE" "Kernel command line"

if [[ -f "$CONFIG_FILE" ]] && grep -Eq '^[[:space:]]*include[[:space:]]+config-user\.txt[[:space:]]*$' "$CONFIG_FILE"; then
    pass "config.txt includes config-user.txt"
else
    fail "config.txt does not include config-user.txt"
fi

if [[ -f "$USER_CONFIG_FILE" ]] && grep -Eq '^[[:space:]]*dtoverlay=uart5[[:space:]]*$' "$USER_CONFIG_FILE"; then
    pass "config-user.txt enables dtoverlay=uart5"
else
    fail "config-user.txt does not contain dtoverlay=uart5"
fi

if [[ -f "$USER_CONFIG_FILE" ]] && grep -Eq '^[[:space:]]*dtoverlay=i2s-dac[[:space:]]*$' "$USER_CONFIG_FILE"; then
    pass "config-user.txt enables dtoverlay=i2s-dac"
else
    fail "config-user.txt does not contain dtoverlay=i2s-dac"
fi

if [[ -e /dev/ttyAMA5 ]]; then
    pass "UART device exists: /dev/ttyAMA5"
else
    fail "UART device is missing: /dev/ttyAMA5"
fi

if [[ -f "$CMDLINE_FILE" ]] && grep -Eq '(^|[[:space:]])console=(serial0|ttyAMA5)' "$CMDLINE_FILE"; then
    fail "A serial console is configured on the UART5 port in cmdline.txt"
else
    pass "No serial console is configured on UART5"
fi

echo
echo "4. Services"
for service_name in uart5_listener.service heartbeat.service; do
    if systemctl cat "$service_name" >/dev/null 2>&1; then
        pass "Service unit exists: $service_name"
    else
        fail "Service unit is missing: $service_name"
        continue
    fi

    if systemctl is-enabled --quiet "$service_name"; then
        pass "Service is enabled: $service_name"
    else
        warn "Service is not enabled: $service_name"
    fi

    if systemctl is-active --quiet "$service_name"; then
        pass "Service is running: $service_name"
    else
        fail "Service is not running: $service_name"
    fi
done

echo
echo "5. MPD and artwork metadata"
if ss -ltn 2>/dev/null | grep -Eq ':[[:space:]]*6600[[:space:]]'; then
    pass "MPD is listening on TCP port 6600"
else
    fail "MPD is not listening on TCP port 6600"
fi

CURRENTSONG_FILE="/var/local/www/currentsong.txt"
if [[ -s "$CURRENTSONG_FILE" ]]; then
    pass "moOde metadata file exists and is not empty: $CURRENTSONG_FILE"
    if grep -q '^coverurl=' "$CURRENTSONG_FILE"; then
        pass "Current-song metadata contains coverurl"
    else
        warn "Current-song metadata does not contain coverurl; play a track or radio station and check again"
    fi
else
    fail "moOde metadata file is missing or empty: $CURRENTSONG_FILE"
    warn "Enable moOde Web UI -> Configure -> Audio -> MPD Options -> Metadata file"
fi

echo
echo "6. Display assets"
if [[ -f /opt/splash.png ]]; then
    pass "Static splash image is installed: /opt/splash.png"
else
    warn "Static splash image is not installed: /opt/splash.png (optional)"
fi

if [[ -d /opt/1024x600 ]] && find /opt/1024x600 -type f -print -quit 2>/dev/null | grep -q .; then
    pass "PeppyMeter assets are installed: /opt/1024x600"
else
    warn "PeppyMeter assets are not installed: /opt/1024x600 (optional unless using PeppyMeter)"
fi

echo
echo "Summary"
echo "-------"
printf 'Passed: %d  Warnings: %d  Failed: %d\n' "$PASS_COUNT" "$WARN_COUNT" "$FAIL_COUNT"

if (( FAIL_COUNT > 0 )); then
    echo "Installation check completed with failures."
    exit 1
elif (( WARN_COUNT > 0 )); then
    echo "Installation check completed with warnings."
    exit 0
else
    echo "Installation check passed."
    exit 0
fi