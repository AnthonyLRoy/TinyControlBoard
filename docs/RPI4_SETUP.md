# UART5 Listener, Display & Boot Configuration on Moode (Raspberry Pi 4)

This guide explains how to clone the repository onto a Raspberry Pi 4 running **Moode Audio**, install required system and Python dependencies, deploy the complete suite of Python companion programs, configure systemd background services, customize boot console settings, and set up splash screen displays (including the animated Plymouth RiverBank theme).

---

## Repo Layout & Python Companion Suite

Companion assets for the Raspberry Pi are tracked in the GitHub repository under [scripts/rpi](scripts/rpi).

### Complete List of Python Programs & Modules (`scripts/rpi/home/`)

| Script File | Purpose / Function |
| :--- | :--- |
| [uart5_listener.py](scripts/rpi/home/antho/uart5_listener.py) | **Primary Service Entry Point:** Receives UART commands from the ESP32, manages display/meter states, and dispatches actions. |
| [heartbeat_sender.py](scripts/rpi/home/antho/heartbeat_sender.py) | **Heartbeat Service Script:** Periodically transmits heartbeat packets over UART5 to inform the ESP32 that the Pi is online. |
| [command_ids.py](scripts/rpi/home/antho/command_ids.py) | **Command Constants:** Defines packet command IDs and message type constants matching the ESP32 protocol. |
| [protocol.py](scripts/rpi/home/antho/protocol.py) | **Protocol Driver:** Low-level binary protocol implementation (packet framing, byte packing/unpacking, and checksum calculation). |
| [uart_writer_client.py](scripts/rpi/home/antho/uart_writer_client.py) | **UART Transmission Client:** Thread-safe client socket wrapper for transmitting outgoing packets back to the ESP32. |
| [mpd_client.py](scripts/rpi/home/antho/mpd_client.py) | **MPD Client Interface:** Low-level socket client for communicating directly with Moode's MPD (Music Player Daemon). |
| [library_browser.py](scripts/rpi/home/antho/library_browser.py) | **Library Browser:** Handles music library directory navigation, search queries, and catalog responses back to the ESP32. |
| [playlist_manager.py](scripts/rpi/home/antho/playlist_manager.py) | **Playlist Manager:** Handles playlist creation, track queuing, and playlist item management. |
| [panel_control.py](scripts/rpi/home/antho/panel_control.py) | **Panel & UI Controller:** Controls Moode UI view switching via Chrome DevTools Protocol (CDP port 9222) and REST/HTTP APIs. |
| [playback_commands.py](scripts/rpi/home/antho/playback_commands.py) | **Playback Controller:** Executes local playback actions (play, pause, next, prev, volume, mute, power). |
| [UAart5Listener.py](scripts/rpi/home/antho/UAart5Listener.py) | **Legacy Wrapper:** Backward-compatibility entry point for launching `uart5_listener.py`. |

### Other Tracked Configuration & Display Assets

- [scripts/rpi/boot/firmware/config.txt](scripts/rpi/boot/firmware/config.txt): Firmware configuration template with `dtoverlay=uart5`
- [scripts/rpi/boot/firmware/config-user.txt](scripts/rpi/boot/firmware/config-user.txt): User overlay configuration template
- [scripts/rpi/boot/firmware/cmdline.txt](scripts/rpi/boot/firmware/cmdline.txt): Boot command line template
- [scripts/rpi/FinalSplashScreen.png](scripts/rpi/FinalSplashScreen.png): Static splash screen image asset
- [scripts/rpi/Peppymeter/](scripts/rpi/Peppymeter/): PeppyMeter needle, background graphics, and `meters.txt` configurations

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

Deploy the Python companion scripts from the cloned repository folder into `/home/<username>/`:

```bash
cp /home/<username>/TinyControlBoard/scripts/rpi/home/*/*.py /home/<username>/
chmod +x /home/<username>/*.py
sudo chown -R <username>:<username> /home/<username>/
```

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

- `pyserial`: Serial port communication over UART5 (`/dev/ttyAMA1` / `/dev/ttyS0`)
- `RPi.GPIO`: Hardware GPIO pin control
- `requests`: HTTP requests for Moode panel control via Chrome DevTools Protocol (CDP port 9222)

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

---

## 4. Configure Systemd Services

### 4.1 UART Listener Service

1. Ensure all Python scripts (`uart5_listener.py`, `protocol.py`, `mpd_client.py`, `library_browser.py`, `playlist_manager.py`, `panel_control.py`, `playback_commands.py`, `command_ids.py`, `uart_writer_client.py`) are present in `/home/<username>/`.

2. Create the systemd service unit:

```bash
sudo nano /etc/systemd/system/uart_listener.service
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
sudo systemctl enable uart_listener.service
sudo systemctl start uart_listener.service
```

4. Check live logs:

```bash
journalctl -u uart_listener.service -f
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
ExecStart=-/sbin/agetty --autologin <username> --noclear %I $TERM
```

*(Replace `<username>` with your actual username, e.g. `pi`).*

---

### 5.2 Hide Kernel Boot Messages & Cursor

Edit the boot command line:

```bash
sudo nano /boot/firmware/cmdline.txt
```

Append the following parameters to the end of the existing single line:

```text
quiet loglevel=0 vt.global_cursor_default=0
```

> **Note:** `/boot/firmware/cmdline.txt` must remain a single continuous line. Do not introduce newlines.

---

## 6. Splash Screen Setup

You can install either the animated **RiverBank Plymouth Theme** (Primary Option) or a simple **Static Image Splash Screen** (Alternative Option).

---

### Option A: RiverBank Plymouth Animated Splash Theme (Recommended)

This installs Plymouth and sets up the RiverBank splash theme (a static banner with an animated blue spinner ring).

#### 1. Copy Theme Folder to the Pi

From your local machine:

```bash
scp -r riverbank-theme pi@<moode-ip>:/tmp/
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

You should see the full-screen RiverBank banner (letterboxed to fit your display without stretching) with a small blue spinner ring rotating near the bottom until Moode services start.

#### 7. Close The Visual Gap Between Plymouth And Moode's UI

By default, Plymouth quits once the boot sequence reaches its normal exit point, which can happen before Moode's kiosk Chromium UI has actually started rendering, leaving a brief black-screen gap.

> **Do not try to delay `plymouth quit` until Chromium/CDP is ready.** On a Pi using **console autologin**, `getty@tty1.service` (which starts the autologin shell that runs `startx`/Chromium) is ordered `After=plymouth-quit-wait.service`, and `plymouth-quit-wait.service` blocks on `plymouth --wait` until Plymouth actually quits. Masking `plymouth-quit.service` and gating the quit on Chromium's CDP port creates a boot deadlock: Chromium can't start until Plymouth quits, and Plymouth won't quit until Chromium is up. If you previously masked `plymouth-quit.service` and installed a custom handoff unit for this, revert it:
> ```bash
> sudo systemctl disable --now moode-plymouth-handoff.service
> sudo systemctl unmask plymouth-quit.service
> sudo systemctl daemon-reload
> ```

Instead, paint the same background Plymouth used as the X root window immediately in `~/.xinitrc`, *before* Chromium launches, so there is no visible gap even though Plymouth quits at its normal (earlier) time:

```bash
# near the top of ~/.xinitrc, before the chromium-browser launch line
feh --bg-scale /usr/share/plymouth/themes/riverbank/background.png &
```

(`feh` must be installed: `sudo apt install -y feh`. If you'd rather not add a dependency, `xsetroot -solid '#050508'` with a color sampled from the theme background works almost as well.)

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

1. Copy the target splash screen image (such as [scripts/rpi/FinalSplashScreen.png](scripts/rpi/FinalSplashScreen.png)) to `/opt/splash.png`:

```bash
sudo cp /home/<username>/FinalSplashScreen.png /opt/splash.png
sudo chmod 644 /opt/splash.png
```

2. Configure your display viewer/framebuffer service or desktop background setting to point to `/opt/splash.png`.

---

## 7. PeppyMeter Configuration

Copy your meter graphics and configuration files into `/opt/1024x600`:

```bash
sudo mkdir -p /opt/1024x600
sudo cp -r /home/<username>/Peppymeter/1024x600/* /opt/1024x600/
```

---

## 8. File Locations & Systemd Services Summary

| Purpose | File Path | Notes |
| :--- | :--- | :--- |
| GitHub Repository | `/home/<username>/TinyControlBoard` | Source tree cloned on Pi |
| Script Suite Location | `/home/<username>/TinyControlBoard/scripts/rpi/home/*/*.py` | 11 Python scripts copied to `/home/<username>/` |
| Firmware Config | `/boot/firmware/config.txt` | Enables `dtoverlay=uart5` |
| Boot Parameters | `/boot/firmware/cmdline.txt` | Single line; includes quiet/splash parameters |
| UART Listener Script | `/home/<username>/uart5_listener.py` | Executable entry point (`chmod +x`) |
| UART Listener Service | `/etc/systemd/system/uart_listener.service` | Auto-starts UART listener on boot |
| Heartbeat Sender Script | `/home/<username>/heartbeat_sender.py` | Sends periodic UART heartbeats |
| Heartbeat Service | `/etc/systemd/system/heartbeat.service` | Auto-starts heartbeat sender |
| Auto-Login Override | `/etc/systemd/system/getty@tty1.service.d/autologin.conf` | Enables console autologin |
| Plymouth RiverBank Theme | `/usr/share/plymouth/themes/riverbank/` | Primary animated splash screen theme |
| Static Splash Image | `/opt/splash.png` | Alternative static splash screen asset |
| PeppyMeter Config | `/opt/1024x600/` | Meter graphics & `meters.txt` config |

---

## 9. Troubleshooting Guide

This section provides diagnostic commands and step-by-step solutions for common issues when deploying the UART5 listener suite and display setup on Moode Audio.

---

### 9.1 UART5 Interface & Hardware Serial Issues

#### Problem: UART listener cannot open or communicate on UART5 (`/dev/ttyAMA1` / `/dev/ttyAMA0`)

1. **Verify firmware overlay is enabled:**
   ```bash
   grep uart5 /boot/firmware/config.txt
   ```
   If missing, add `dtoverlay=uart5` to `/boot/firmware/config.txt` and reboot.

2. **Check loaded device tree overlays:**
   ```bash
   dtoverlay -l
   ```
   You should see `uart5` listed in the active overlays.

3. **Verify serial tty device nodes:**
   ```bash
   ls -l /dev/ttyAMA* /dev/ttyS* 2>/dev/null
   ```
   *Note:* On Raspberry Pi 4, `dtoverlay=uart5` typically creates `/dev/ttyAMA1` (TX on GPIO 12, RX on GPIO 13).

4. **Check kernel log for serial initialization:**
   ```bash
   dmesg | grep -i tty
   ```

5. **Ensure serial console is not claiming the UART port:**
   Open `/boot/firmware/cmdline.txt` and verify that no `console=serial0...` or `console=ttyAMA1...` parameters exist. Remove any serial console arguments to prevent serial port conflict.

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

#### Problem: `uart_listener.service` or `heartbeat.service` fails to start or keeps restarting

1. **Inspect live service status and journal logs:**
   ```bash
   sudo systemctl status uart_listener.service -l
   journalctl -u uart_listener.service -n 50 --no-pager
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

3. **`PermissionDeniedError` or `Exec format error`:**
   Ensure executable permissions and user ownership are set correctly:
   ```bash
   chmod +x /home/<username>/*.py
   sudo chown -R <username>:<username> /home/<username>/
   ```

4. **Reload systemd after modifying service unit files:**
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl restart uart_listener.service
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

### 9.4 Moode Panel Switching & Chrome DevTools Protocol (CDP) Issues

#### Problem: Panel switching commands fail or `panel_control.py` cannot connect to Chrome DevTools Protocol

1. **Test CDP endpoint availability:**
   ```bash
   curl http://localhost:9222/json
   ```
   This should return a JSON array listing the active browser targets/tabs.

2. **If `curl` returns `Connection refused`:**
   - Moode UI kiosk browser (Chromium) must be running on local display.
   - Verify Chromium start script includes `--remote-debugging-port=9222`.

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

#### Problem: Toggling the Meter button (app or physical) sends the UART command fine (`Handling Command ID: 0x0115` in `uart5_listener` logs), but the display never switches to the peppy meter. `sudo moodeutl --setdisplay peppy` on the RPi prints:

```
This option requires PeppyALSA driver to be On
```

**Root cause:** moOde's `enable_peppyalsa` can fail if there is a curruption on the service

In the **Peppy Display** and **PeppyALSA driver** toggles under **Configure → Peripherals → Local Display** are greyed out/disabled whenever **Local Display** (WebUI shown on the local screen) is **On** — the two modes are mutually exclusive. Our RPi `toggle_meter_display()` handler runs `moodeutl --setdisplay webui` when the meter is turned OFF, which sets `local_display=1`, `peppy_display=0` — this is expected, but it means Local Display stays "On" until you explicitly re-enable Peppy in the UI.

**Fix:**
1. Open the moOde web UI → **Configure → Peripherals → Local Display**.
2. Turn **Local Display** (WebUI) **Off** first — this un-greys the Peppy controls.
3. Turn **PeppyALSA driver** **On**.
4. Turn **Peppy Display** **On**.
5. Save. `sudo moodeutl --setdisplay peppy` (and the UART meter toggle) should now work again.

**Quick diagnostic commands:**
```bash
# Confirm the UART command is actually arriving and being handled
sudo journalctl -u uart5_listener -n 50 --no-pager

# Check current relevant moOde settings
sqlite3 /var/local/www/db/moode-sqlite3.db 'select param, value from cfg_system;' \
  | grep -E 'local_display|peppy_display|audioout|alsaequal|eqfa12p'
```

Note: `enable_peppyalsa` won't show up in that dump — it's session-only, so expect to have to re-enable it after a reboot or moOde update if the meter view stops responding again.
