# UART5 Listener, Display & Boot Configuration on moOde (Raspberry Pi 4)

This guide explains how to clone the repository onto a Raspberry Pi 4 running **moOde Audio**, install the required system and Python dependencies, deploy the Python companion programs, configure systemd services, customize boot console settings, and set up splash screens, including the animated Plymouth RiverBank theme.

---

## Repo Layout & Python Companion Suite

Companion assets for the Raspberry Pi are tracked in the GitHub repository under [scripts/rpi](../../scripts/rpi).

### Complete List of Python Programs & Modules (`scripts/rpi/home/`)

| Script File | Purpose / Function |
| :--- | :--- |
| [uart5_listener.py](../../scripts/rpi/home/antho/uart5_listener.py) | **Primary Service Entry Point:** Receives UART commands from the ESP32, manages display/meter states, and dispatches actions. |
| [heartbeat_sender.py](../../scripts/rpi/home/antho/heartbeat_sender.py) | **Heartbeat Service Script:** Periodically transmits heartbeat packets over UART5 to inform the ESP32 that the Pi is online. |
| [command_ids.py](../../scripts/rpi/home/antho/command_ids.py) | **Command Constants:** Defines packet command IDs and message type constants matching the ESP32 protocol. |
| [protocol.py](../../scripts/rpi/home/antho/protocol.py) | **Protocol Driver:** Low-level binary protocol implementation (packet framing, byte packing/unpacking, and checksum calculation). |
| [uart_writer_client.py](../../scripts/rpi/home/antho/uart_writer_client.py) | **UART Transmission Client:** Thread-safe client socket wrapper for transmitting outgoing packets back to the ESP32. |
| [mpd_client.py](../../scripts/rpi/home/antho/mpd_client.py) | **MPD Client Interface:** Low-level socket client for communicating directly with moOde's MPD (Music Player Daemon). |
| [library_browser.py](../../scripts/rpi/home/antho/library_browser.py) | **Library Browser:** Handles music library directory navigation, search queries, and catalog responses back to the ESP32. |
| [playlist_manager.py](../../scripts/rpi/home/antho/playlist_manager.py) | **Playlist Manager:** Handles playlist creation, track queuing, and playlist item management. |
| [panel_control.py](../../scripts/rpi/home/antho/panel_control.py) | **Panel & UI Controller:** Controls moOde view switching through the Chrome DevTools Protocol (CDP port 9222). |
| [playback_commands.py](../../scripts/rpi/home/antho/playback_commands.py) | **Playback Controller:** Executes local playback actions (play, pause, next, prev, volume, mute, power). |
| [UAart5Listener.py](../../scripts/rpi/home/antho/UAart5Listener.py) | **Legacy Wrapper:** Backward-compatibility entry point for launching `uart5_listener.py`; the unusual filename is retained for compatibility. |

### Other Tracked Configuration & Display Assets

- [scripts/rpi/boot/firmware/config.txt](../../scripts/rpi/boot/firmware/config.txt): Firmware configuration template with the `config-user.txt` include
- [scripts/rpi/boot/firmware/config-user.txt](../../scripts/rpi/boot/firmware/config-user.txt): User overlay configuration template containing `dtoverlay=uart5`
- [scripts/rpi/boot/firmware/cmdline.txt](../../scripts/rpi/boot/firmware/cmdline.txt): Boot command-line template
- [scripts/rpi/FinalSplashScreen.png](../../scripts/rpi/FinalSplashScreen.png): Static splash-screen image asset
- [scripts/rpi/Peppymeter/](../../scripts/rpi/Peppymeter/): PeppyMeter needle, background graphics, and `meters.txt` configurations

---

## 1. Clone Repository & Install System Dependencies

> **Note on Username:** Throughout this guide, replace `<username>` with your actual Raspberry Pi user account name (for example `pi` or your custom login name).

### 1.1 Clone the GitHub Repository on the Pi

Log into your Raspberry Pi over SSH and clone the repository into your home directory (`/home/<username>`):

```bash
cd /home/<username>
git clone https://github.com/<your-username>/TinyControlBoard.git
```

> *(Replace `<your-username>` with your actual GitHub username or repository URL).*

Deploy the Python companion scripts from the cloned repository into `/home/<username>/`:

```bash
cp /home/<username>/TinyControlBoard/scripts/rpi/home/*/*.py /home/<username>/
sudo chown -R <username>:<username> /home/<username>/
```

Run the read-only installation checker from the repository root. It reports missing files, packages, configuration, services, UART devices, MPD, artwork metadata, and display assets:

```bash
cd /home/<username>/TinyControlBoard
bash scripts/rpi/check_installation.sh
```

The checker exits with status `1` if it finds a failure and status `0` if it finds only passes or warnings. It does not modify the Pi. Warnings for `/opt/splash.png`, PeppyMeter assets, or missing `coverurl` can be expected when those optional features are not currently being used or nothing is playing.

---

### 1.2 Install System Utility Packages

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y git ddcutil
```

---

## 2. Enable UART5 Interface

The tracked firmware configuration uses `config.txt` to include a separate user configuration file. The UART5 overlay is defined in `config-user.txt`, which is loaded by this line in `config.txt`:

```text
include config-user.txt
```

1. Open the user firmware configuration file:

```bash
sudo nano /boot/firmware/config-user.txt
```

2. Add or confirm the UART5 overlay:

```text
dtoverlay=uart5
```

Do not add a second `dtoverlay=uart5` line to `config.txt`; the existing `include config-user.txt` line loads it automatically.

3. Reboot to apply changes:

```bash
sudo reboot
```

---

## 3. Install Required Python Dependencies

The Python companion script suite relies on three external Python modules:

- `pyserial`: Serial port communication over UART5 (`/dev/ttyAMA5`)
- `RPi.GPIO`: Hardware GPIO pin control
- `requests`: HTTP requests used to discover the Chrome DevTools Protocol endpoint (CDP port 9222)

### Option 3.1: Distro-Managed System Packages (Recommended)

Install all required Python packages using `apt`:

```bash
sudo apt update
sudo apt install -y python3-serial python3-rpi.gpio python3-requests
```

### Option 3.2: Pip Package Installation

If using `pip`:

```bash
sudo apt install -y python3-pip
sudo python3 -m pip install pyserial RPi.GPIO requests
```

Verify package installations:

```bash
python3 -c "import serial, RPi.GPIO, requests; print('All Python dependencies installed successfully!')"
```

### 3.3 Enable moOde Metadata for Android Cover Artwork

The Android main screen reads the currently playing artwork from moOde's current-song metadata. In current moOde 10.x, enable **Metadata file** before expecting cover artwork to appear in the Android app:

1. Open the moOde Web UI.
2. Go to **Menu** → **Configure** → **Audio** → **MPD Options**.
3. Set **Metadata file** to **On**.
4. Save or apply the configuration.

Verify that moOde is generating the metadata file:

```bash
cat /var/local/www/currentsong.txt
```

The output is normally stored as `key=value` lines and should include fields such as `file`, `artist`, `album`, `title`, `coverurl`, and `state`. For example:

```text
file=RADIO/Example Station.pls
artist=Radio station
album=Example Station
title=Current programme or track
coverurl=imagesw%2Fradio-logos%2FExample%20Station.jpg
state=play
```

The Android app uses `coverurl` for radio station logos and changing radio-track artwork. It checks the current metadata periodically, so the displayed image can change while the same station remains selected. Normal music artwork continues to use moOde's `coverart.php` endpoint.

---

## 4. Configure Systemd Services

### 4.1 UART Listener Service

1. Ensure all required Python scripts and modules are present in `/home/<username>/`.

2. Create the systemd service unit:

```bash
sudo nano /etc/systemd/system/uart5_listener.service
```

```ini
[Unit]
Description=UART5 Listener for ESP32
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/<username>/uart5_listener.py
Restart=always
User=<username>
Group=<username>
WorkingDirectory=/home/<username>
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

3. Enable and start the service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable uart5_listener.service
sudo systemctl start uart5_listener.service
```

4. Check live logs:

```bash
journalctl -u uart5_listener.service -f
```

---

### 4.2 Heartbeat Sender Service

1. Ensure `heartbeat_sender.py` is present in `/home/<username>/`.
2. Create the heartbeat service unit:

```bash
sudo nano /etc/systemd/system/heartbeat.service
```

```ini
[Unit]
Description=UART5 Heartbeat Sender
After=network.target multi-user.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/<username>/heartbeat_sender.py
Restart=always
User=<username>
Group=<username>
WorkingDirectory=/home/<username>
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

3. Enable and start the service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable heartbeat.service
sudo systemctl start heartbeat.service
```

4. Check logs:

```bash
journalctl -u heartbeat.service -f
```

---

## 5. Console Auto-Login & Boot Text Customization

### 5.1 Enable Console Auto-Login

Choose one of the following methods to enable automatic login on the local console:

#### Method A: Raspi-Config Utility

```bash
sudo raspi-config
```

Navigate to: **System Options** → **Boot / Auto Login** → **B2 Console Autologin** (or **B4 Desktop Autologin**).

#### Method B: Systemd Getty Override

```bash
sudo mkdir -p /etc/systemd/system/getty@tty1.service.d
sudo nano /etc/systemd/system/getty@tty1.service.d/autologin.conf
```

Add the following override content:

```ini
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin <username> --noissue --nohostname --noclear %I $TERM
```

*(Replace `<username>` with your actual username, e.g. `pi`).*

The `--noissue` option suppresses the `/etc/issue` banner, and `--nohostname` suppresses the `hostname login:` text. Reload systemd after changing the override:

```bash
sudo systemctl daemon-reload
sudo systemctl restart getty@tty1.service
```

The `moOde (automatic login)` message is printed by `agetty` itself and may still flash briefly. The steps below suppress the remaining login and boot output.

### 5.2 Silence Post-Login Output

Create `.hushlogin` for the autologin user and clear the message of the day:

```bash
touch ~/.hushlogin
sudo truncate -s 0 /etc/motd
```

If you also want to remove the shell prompt from tty1, append this line to `~/.bash_profile`:

```bash
[ "$(tty)" = /dev/tty1 ] && clear
```

---

### 5.3 Hide Kernel Boot Messages & Cursor

Edit the boot command line:

```bash
sudo nano /boot/firmware/cmdline.txt
```

Keep the file on one line. If it contains `console=tty1`, change that entry to `console=tty3` so boot output is sent to an unused virtual terminal. Also make sure the following parameters are present:

```text
console=tty3 quiet loglevel=0 logo.nologo vt.global_cursor_default=0
```

> **Note:** `/boot/firmware/cmdline.txt` must remain a single continuous line. Do not introduce newlines.

### 5.4 Optional: Disable the tty1 Getty

If moOde's local display is started by its own systemd service and nothing depends on the tty1 login session, autologin may not be needed. Disable and mask the tty1 getty:

```bash
sudo systemctl disable getty@tty1.service
sudo systemctl mask getty@tty1.service
```

This leaves tty2 through tty6 and SSH available for recovery. To restore tty1 later:

```bash
sudo systemctl unmask getty@tty1.service
sudo systemctl enable getty@tty1.service
sudo systemctl start getty@tty1.service
```

> **Caution:** Masking `getty@tty1.service` removes the tty1 login entirely. Use this only after confirming that moOde's local display does not depend on the tty1 session.

---

## 6. Splash Screen Setup

You can install either the animated **RiverBank Plymouth Theme** (Primary Option) or a simple **Static Image Splash Screen** (Alternative Option).

---

### Option A: RiverBank Plymouth Animated Splash Theme (Recommended)

This installs Plymouth and sets up the RiverBank splash theme (a static banner with an animated blue spinner ring).

#### 1. Copy Theme Folder to the Pi

From the repository root on your local machine:

```bash
scp -r scripts/rpi/riverbank-plymouth-theme/riverbank-theme pi@<moode-ip>:/tmp/
```

*(Replace `pi` and `<moode-ip>` with your actual username and Pi IP address).*

#### 2. Install Plymouth Prerequisites

```bash
sudo apt update
sudo apt install -y plymouth plymouth-themes
```

#### 3. Deploy Theme Files

```bash
sudo mkdir -p /usr/share/plymouth/themes/riverbank
sudo cp /tmp/riverbank-theme/background.png /usr/share/plymouth/themes/riverbank/
sudo cp /tmp/riverbank-theme/spinner.png /usr/share/plymouth/themes/riverbank/
sudo cp /tmp/riverbank-theme/riverbank.plymouth /usr/share/plymouth/themes/riverbank/
sudo cp /tmp/riverbank-theme/riverbank.script /usr/share/plymouth/themes/riverbank/
```

#### 4. Enable Splash Mode in Firmware Boot Config

Edit `/boot/firmware/cmdline.txt` (must remain a single line) and append:

```text
splash quiet plymouth.ignore-serial-consoles
```

#### 5. Set Default Theme & Rebuild Initramfs

```bash
sudo plymouth-set-default-theme -R riverbank
```

*(Note: If your Plymouth version does not support `-R`, run `sudo plymouth-set-default-theme riverbank` followed by `sudo update-initramfs -u`).*

#### 6. Reboot & Verify

```bash
sudo reboot
```

You should see the full-screen RiverBank banner (letterboxed to fit your display without stretching) with a small blue spinner ring rotating near the bottom until moOde services start.

#### 7. Close the Visual Gap Between Plymouth and moOde's UI

By default, Plymouth quits once the boot sequence reaches its normal exit point, which can happen before moOde's kiosk UI has started rendering, leaving a brief black-screen gap. The clean way to close this gap is to keep Plymouth's splash on screen (instead of letting it quit early) and only tell it to quit — while **retaining** the last splash frame in the framebuffer — right after your own splash image (`feh`) has been drawn over it. This removes the black frame entirely instead of racing to paint over it.

**a. Stop Plymouth from quitting on its own:**

```bash
sudo systemctl mask plymouth-quit.service plymouth-quit-wait.service
```

The splash now stays up until explicitly told to quit — step **c** below does that. Don't skip step **c** or the boot will appear stuck on the splash screen indefinitely.

**b. Start X on the same VT, without painting a background:**

Wherever your session normally calls `startx` (autologin shell profile, or the unit that launches the local UI), use:

```bash
startx -- vt1 -keeptty -nocursor -background none
```

`-background none` stops the X server from painting the root window black, so the retained Plymouth splash frame stays visible underneath until `feh` draws over it.

**c. Quit Plymouth only after `feh` has drawn**, in `~/.xinitrc`:

```bash
xset s off -dpms s noblank
feh --fullscreen --hide-pointer --no-fehbg /usr/share/plymouth/themes/riverbank/background.png &
sleep 0.3
sudo /usr/bin/plymouth quit --retain-splash
exec /path/to/your-ui   # e.g. the chromium-browser kiosk launch line
```

`--retain-splash` leaves the last splash frame in the framebuffer instead of clearing it, so there's no black frame during handover. The short `sleep 0.3` gives `feh` time to finish drawing before Plymouth's frame is released.

(`feh` must be installed: `sudo apt install -y feh`.)

> **Important:** The `exec` line above ultimately runs moOde's WebUI launch block further down in `~/.xinitrc` (under `# Launch WebUI or Peppy` / `if [ $WEBUI_SHOW = "1" ]`). That block **must** include `--remote-debugging-port=9222` on the `chromium` command, or `panel_control.py`'s menu/panel switching (which drives Chromium over the Chrome DevTools Protocol) will silently do nothing:
>
> ```bash
> # Launch WebUI or Peppy
> if [ $WEBUI_SHOW = "1" ]; then
> 	# Clear browser cache
> 	$(/var/www/util/sysutil.sh clearbrcache)
> 	# Launch chromium browser
> 	chromium \
> 	--app="http://localhost/" \
> 	--window-size="$SCREEN_RES" \
> 	--window-position="0,0" \
> 	--enable-features="OverlayScrollbar" \
> 	--no-first-run \
>         --remote-debugging-port=9222 \
> 	--disable-infobars \
> 	--disable-session-crashed-bubble \
> 	--kiosk
> ```
>
> If you're editing `~/.xinitrc` for the first time (or restoring it after a moOde update overwrote it), double-check this flag is still present — moOde's stock `.xinitrc` does not include it by default.

Add a sudoers rule so `plymouth quit` doesn't prompt for a password (replace `<username>` with your autologin user):

```bash
echo '<username> ALL=(root) NOPASSWD: /usr/bin/plymouth' | sudo tee /etc/sudoers.d/plymouth
```

**d. Diagnose any remaining delay.** If the gap is still long after the above, X is simply starting late rather than flickering:

```bash
systemd-analyze blame | head -20
systemd-analyze critical-chain localui.service
```

Look for `network-online.target`, `nginx`, or `mpd` in the chain. Moving your local-UI unit to `After=systemd-user-sessions.service` and dropping any `Wants=network-online.target` can pull X startup forward by several seconds.

To watch the exact handover timing without a full reboot cycle:

```bash
sudo plymouthd --debug
```

Writes to `/var/log/plymouth-debug.log`.

#### Plymouth Notes & Previewing

- **Animation Concept:** The background stays static; only the small ring icon animates as a waiting indicator on top of the banner.
- **Console Previewing (without rebooting):** From a real physical console on the Pi (not SSH), run:
  ```bash
  sudo plymouthd --debug --no-daemon &
  sudo plymouth --show-splash
  ```
  To exit the preview:
  ```bash
  sudo plymouth --quit
  ```
- **Raspberry Pi OS Trixie Bug:** If affected by the known Trixie graphics regression, you may see 3 flashing dots instead. Always test on a physical display with a full reboot.

---

### Option B: Static Image Splash Screen (Alternative)

If you prefer a simple static image splash screen without Plymouth:

1. From the repository root on the Pi, copy the target splash screen image (such as [scripts/rpi/FinalSplashScreen.png](../../scripts/rpi/FinalSplashScreen.png)) to `/opt/splash.png`:

```bash
sudo cp /home/<username>/TinyControlBoard/scripts/rpi/FinalSplashScreen.png /opt/splash.png
sudo chmod 644 /opt/splash.png
```

2. Configure your display viewer/framebuffer service or desktop background setting to point to `/opt/splash.png`.

---

## 7. PeppyMeter Configuration

Copy the meter graphics and configuration files from the cloned repository into `/opt/1024x600`:

```bash
sudo mkdir -p /opt/1024x600
sudo cp -r /home/<username>/TinyControlBoard/scripts/rpi/Peppymeter/1024x600/* /opt/1024x600/
```

---

## 8. File Locations & Systemd Services Summary

| Purpose | File Path | Notes |
| :--- | :--- | :--- |
| GitHub Repository | `/home/<username>/TinyControlBoard` | Source tree cloned on Pi |
| Script Suite Location | `/home/<username>/TinyControlBoard/scripts/rpi/home/*/*.py` | 11 Python scripts copied to `/home/<username>/` |
| Firmware Config | `/boot/firmware/config-user.txt` | Defines `dtoverlay=uart5`, included by `config.txt` |
| Boot Parameters | `/boot/firmware/cmdline.txt` | Single line; includes quiet/splash parameters |
| UART Listener Script | `/home/<username>/uart5_listener.py` | Python entry point used by systemd |
| UART Listener Service | `/etc/systemd/system/uart5_listener.service` | Auto-starts UART listener on boot |
| Heartbeat Sender Script | `/home/<username>/heartbeat_sender.py` | Sends periodic UART heartbeats |
| Heartbeat Service | `/etc/systemd/system/heartbeat.service` | Auto-starts heartbeat sender |
| Auto-Login Override | `/etc/systemd/system/getty@tty1.service.d/autologin.conf` | Enables console autologin |
| Plymouth RiverBank Theme | `/usr/share/plymouth/themes/riverbank/` | Primary animated splash screen theme |
| Static Splash Image | `/opt/splash.png` | Alternative static splash screen asset |
| Plymouth Sudoers Rule | `/etc/sudoers.d/plymouth` | Lets `plymouth quit` run passwordless from `.xinitrc` |
| PeppyMeter Config | `/opt/1024x600/` | Meter graphics & `meters.txt` config |

---

## 9. Troubleshooting Guide

This section provides diagnostic commands and step-by-step solutions for common issues when deploying the UART5 listener suite and display setup on moOde Audio.

---

### 9.1 UART5 Interface & Hardware Serial Issues

#### Problem: UART listener cannot open or communicate on UART5 (`/dev/ttyAMA5`)

1. **Verify firmware overlay is enabled:**
   ```bash
   grep -n "include config-user.txt" /boot/firmware/config.txt
   grep -n "dtoverlay=uart5" /boot/firmware/config-user.txt
   ```
   If either line is missing, restore the include in `config.txt` and the overlay in `config-user.txt`, then reboot.

2. **Check loaded device tree overlays:**
   ```bash
   dtoverlay -l
   ```
   You should see `uart5` listed in the active overlays.

3. **Verify serial tty device nodes:**
   ```bash
   ls -l /dev/ttyAMA* /dev/ttyS* 2>/dev/null
   ```
   The project scripts expect `/dev/ttyAMA5`. Confirm that this device node exists before starting the services.

4. **Check kernel log for serial initialization:**
   ```bash
   dmesg | grep -i tty
   ```

5. **Ensure serial console is not claiming the UART port:**
   Open `/boot/firmware/cmdline.txt` and verify that no `console=serial0...` or `console=ttyAMA5...` parameters exist. Remove any serial console arguments to prevent a serial-port conflict.

6. **Fix `Permission denied` errors accessing serial port:**
   Ensure your user is in the `dialout` group:
   ```bash
   sudo usermod -a -G dialout <username>
   sudo reboot
   ```

7. **Verify physical wiring (RPi 4 to ESP32):**
   - **RPi 4 GPIO 12 (TX5)** $\rightarrow$ **ESP32 RX**
   - **RPi 4 GPIO 13 (RX5)** $\rightarrow$ **ESP32 TX**
   - **GND** $\rightarrow$ **GND** (Common Ground required)
   - Baud rate: Default is **921600 baud** (must match ESP32 configuration).

#### Problem: `uart5_listener.service` fails to start after a moOde update

A moOde update can replace `/boot/firmware/config.txt` and remove the line that includes the user configuration file. Check both files:

```bash
grep -n "include config-user.txt" /boot/firmware/config.txt
grep -n "dtoverlay=uart5" /boot/firmware/config-user.txt
```

If the include line is missing, add this line to `/boot/firmware/config.txt`:

```text
include config-user.txt
```

Do not duplicate `dtoverlay=uart5` in `config.txt`; keep the overlay in `/boot/firmware/config-user.txt`. Reboot after restoring the configuration, then reconnect and check the service:

```bash
sudo reboot
sudo systemctl status uart5_listener.service -l
journalctl -u uart5_listener.service -n 50 --no-pager
```

---

### 9.2 systemd Service & Python Script Execution Failures

#### Problem: `uart5_listener.service` or `heartbeat.service` fails to start or keeps restarting

1. **Inspect live service status and journal logs:**
   ```bash
   sudo systemctl status uart5_listener.service -l
   journalctl -u uart5_listener.service -n 50 --no-pager
   ```

2. **`ModuleNotFoundError: No module named '...'`**
   If logs indicate a missing module:
   - For `serial` (PySerial), `RPi.GPIO`, or `requests`:
     ```bash
     sudo apt install -y python3-serial python3-rpi.gpio python3-requests
     ```
   - For internal companion modules (e.g., `ModuleNotFoundError: No module named 'protocol'` or `'command_ids'`):
     Ensure **all 11 Python files** from the repository's `scripts/rpi/home/` folder have been copied into `/home/<username>/`:
     ```bash
     cp /home/<username>/TinyControlBoard/scripts/rpi/home/*/*.py /home/<username>/
     ls -la /home/<username>/*.py
     ```

3. **`PermissionError` or a file ownership error:**
   Ensure the service user can read the scripts and access the UART device:
   ```bash
   sudo chown -R <username>:<username> /home/<username>/
   sudo usermod -a -G dialout <username>
   ```
   The service invokes `/usr/bin/python3` directly, so executable permissions on the `.py` files are not required.

4. **Reload systemd after modifying service unit files:**
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl restart uart5_listener.service
   sudo systemctl restart heartbeat.service
   ```

---

### 9.3 MPD (Music Player Daemon) Connection Failures

#### Problem: Listener logs show `ConnectionRefusedError` or socket timeout connecting to MPD on port 6600

1. **Verify MPD service status:**
   ```bash
   sudo systemctl status mpd
   ```

2. **Check if MPD is listening on port 6600:**
   ```bash
   ss -tulpn | grep 6600
   ```
   *(or `netstat -tulpn | grep 6600`)*

3. **Restart MPD service if needed:**
   ```bash
   sudo systemctl restart mpd
   ```

---

### 9.4 moOde Panel Switching & Chrome DevTools Protocol (CDP) Issues

#### Problem: Panel switching commands fail or `panel_control.py` cannot connect to Chrome DevTools Protocol

1. **Test CDP endpoint availability:**
   ```bash
   curl http://localhost:9222/json
   ```
   This should return a JSON array listing the active browser targets/tabs.

2. **If `curl` returns `Connection refused`:**
   - The moOde UI kiosk browser (Chromium) must be running on the local display.
   - Verify Chromium start script includes `--remote-debugging-port=9222`.
   - This flag lives on the `chromium` command in the `# Launch WebUI or Peppy` block of `~/.xinitrc` (inside `if [ $WEBUI_SHOW = "1" ]`). moOde's stock `.xinitrc` does not enable it by default, and it can also get dropped again after a moOde update overwrites `.xinitrc` — re-check it any time panel switching stops working after an update.

3. **Manually test panel switching script in terminal:**
   ```bash
   python3 -c "import panel_control as p; p.switch_panel(0)"
   ```

---

### 9.5 Plymouth Splash Screen & Boot Display Issues

#### Problem: Splash screen does not display, or shows 3 spinning dots instead of RiverBank theme

1. **Verify `/boot/firmware/cmdline.txt` formatting:**
   - Must be a **single continuous line** (no line breaks).
   - Must contain: `splash quiet plymouth.ignore-serial-consoles`.
   - Check with:
     ```bash
     cat /boot/firmware/cmdline.txt
     ```

2. **Rebuild initramfs to ensure Plymouth theme is included in boot RAM disk:**
   ```bash
   sudo plymouth-set-default-theme -R riverbank
   ```
   If `-R` is unsupported by your Plymouth version, run:
   ```bash
   sudo plymouth-set-default-theme riverbank
   sudo update-initramfs -u
   ```

3. **Test Plymouth theme locally on console (non-SSH):**
   ```bash
   sudo plymouthd --debug --no-daemon &
   sudo plymouth --show-splash
   ```
   To exit preview:
   ```bash
   sudo plymouth --quit
   ```

4. **Raspberry Pi OS Trixie / Kernel DRM Graphics Bug:**
   If using newer Debian/Trixie kernel builds where simpledrm/kms driver initialization delays KMS output, Plymouth may default to text or 3-dot mode. A full reboot with physical display attached is required for accurate verification.

#### Problem: Boot appears stuck on the splash screen and never reaches the UI

- This happens if `plymouth-quit.service` / `plymouth-quit-wait.service` were masked (per step 7a above) but the `sudo plymouth quit --retain-splash` call in `~/.xinitrc` never runs or fails silently.
- Check that `sudoers.d/plymouth` is in place so the call doesn't block on a password prompt: `sudo cat /etc/sudoers.d/plymouth`.
- Check that `startx` is actually reaching `.xinitrc` (X may be failing to start — check `~/.local/share/xorg/Xorg.0.log` or run `startx -- vt1 -keeptty -nocursor -background none` manually from a console).
- As a recovery step, unmask the services and reboot: `sudo systemctl unmask plymouth-quit.service plymouth-quit-wait.service`.

#### Problem: Brief black screen still appears between Plymouth and the UI

- Confirm `plymouth-quit.service`/`plymouth-quit-wait.service` are masked (`systemctl is-enabled plymouth-quit.service` should show `masked`).
- Confirm `startx` is passed `-background none` — without it, X paints the root window black before `feh` draws.
- Increase the `sleep 0.3` in `.xinitrc` slightly if `feh` is drawing after the `plymouth quit --retain-splash` call rather than before it.
- Use `systemd-analyze blame` / `systemd-analyze critical-chain localui.service` (or whatever unit starts your local UI) to check whether X itself is starting late rather than flickering.

---

### 9.6 Console Auto-Login Issues

#### Problem: System stops at login prompt on `tty1` instead of auto-logging in

1. **Verify getty auto-login override:**
   ```bash
   cat /etc/systemd/system/getty@tty1.service.d/autologin.conf
   ```
2. **Ensure username matches your active account:**
   ```ini
   [Service]
   ExecStart=
   ExecStart=-/sbin/agetty --autologin <username> --noclear %I $TERM
   ```
   *(Replace `<username>` with `pi` or your actual login username).*

3. **Reload systemd and restart getty:**
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl restart getty@tty1.service
   ```

---

### 9.7 Meter Button/UART Command Does Nothing (Peppy Display Won't Switch)

#### Problem: Toggling the Meter button (app or physical) sends the UART command successfully (`Handling Command ID: 0x0115` appears in the `uart5_listener` logs), but the display never switches to the PeppyMeter. `sudo moodeutl --setdisplay peppy` on the Pi prints:

```
This option requires PeppyALSA driver to be On
```

**Root cause:** The PeppyALSA configuration is unavailable or failed to start.

The **Peppy Display** and **PeppyALSA driver** toggles under **Configure → Peripherals → Local Display** are greyed out or disabled whenever **Local Display** (the Web UI shown on the local screen) is **On**; the two modes are mutually exclusive. The RPi `toggle_meter_display()` handler runs `moodeutl --setdisplay webui` when the meter is turned off, which sets `local_display=1` and `peppy_display=0`. This is expected, but it means Local Display stays on until you explicitly re-enable PeppyMeter in the UI.

**Fix:**
1. Open the moOde Web UI → **Configure → Peripherals → Local Display**.
2. Turn **Local Display** (WebUI) **Off** first — this un-greys the Peppy controls.
3. Turn **PeppyALSA driver** **On**.
4. Turn **Peppy Display** **On**.
5. Save. `sudo moodeutl --setdisplay peppy` (and the UART meter toggle) should now work again.

**Quick diagnostic commands:**
```bash
# Confirm the UART command is actually arriving and being handled
   sudo journalctl -u uart5_listener.service -n 50 --no-pager

# Check current relevant moOde settings
sqlite3 /var/local/www/db/moode-sqlite3.db 'select param, value from cfg_system;' \
  | grep -E 'local_display|peppy_display|audioout|alsaequal|eqfa12p'
```

Note: `enable_peppyalsa` won't show up in that dump — it's session-only, so expect to have to re-enable it after a reboot or moOde update if the meter view stops responding again.
