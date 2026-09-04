# UART5 Listener Setup on Moode (Raspberry Pi)

This guide explains how to enable UART5 on a Raspberry Pi running **Moode**, install required dependencies, and configure **systemd services** to automatically run:

- A UART5 listener
- A UART heartbeat sender

It also covers auto-login configuration and troubleshooting.

## Repo Layout

This repo keeps Raspberry Pi companion assets under `scripts/rpi/`.

Important tracked files:

- `scripts/rpi/home/antho/uart5_listener.py`: Pi-side UART listener entry point
- `scripts/rpi/home/antho/protocol.py`, `uart_writer_client.py`, `mpd_client.py`, `library_browser.py`, `panel_control.py`, `playback_commands.py`, and `command_ids.py`: listener modules deployed alongside the entry point
- `scripts/rpi/home/antho/heartbeat_sender.py`: Pi-side heartbeat sender
- `scripts/rpi/boot/firmware/config.txt`: example firmware config with `dtoverlay=uart5`
- `scripts/rpi/boot/firmware/config-user.txt`: example user overlay additions
- `scripts/rpi/boot/firmware/cmdline.txt`: example boot command line

Deployment note:

- These files are source-controlled examples, not their final runtime locations on the Pi.
- Copy the scripts and config fragments from this folder into the deployment paths referenced below when setting up a Pi.

Current listener behavior note:

- On startup, the UART listener forces the display to the firmware default meter-off state (`webui`) before processing incoming commands.

---

## 1. Install Moode Using Raspberry Pi Imager

Use the official **Raspberry Pi Imager** to flash Moode onto your SD card.

Once Moode is running, update the system:

```bash
sudo apt update
sudo apt upgrade -y
```

---

## 2. Install Required Packages

```bash
sudo apt install -y ddcutil
```

---

## 3. Enable UART5

Edit the Raspberry Pi firmware configuration:

```bash
sudo nano /boot/firmware/config.txt
```

Add or confirm the following entry:

```text
dtoverlay=uart5
```

Reboot to apply:

```bash
sudo reboot
```

---

## 4. Install Python Serial Support

### 4.1 Preferred Installation (pip)

```bash
sudo python3 -m pip install pyserial
```

Verify installation:

```bash
python3 -c "import serial; print(serial.__version__)"
```

---

### 4.2 If `pip` Is Missing

```bash
sudo apt install -y python3-pip
sudo python3 -m pip install pyserial
```

---

### 4.3 If You Encounter `externally-managed-environment`

Install the distro-managed version instead:

```bash
sudo apt install -y python3-serial
```

---

## 5. Create the UART Listener systemd Service

```bash
sudo nano /etc/systemd/system/uart_listener.service
```

```ini
[Unit]
Description=UART5 Listener for ESP32
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/antho/uart5_listener.py
Restart=always
User=antho
Group=antho
WorkingDirectory=/home/antho
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

---

## 6. Create the UART Listener Script

```bash
nano /home/antho/uart5_listener.py
```

Paste the contents of your UART listener script.

Set permissions:

```bash
chmod +x /home/antho/uart5_listener.py
sudo chown antho:antho /home/antho/uart5_listener.py
```

---

## 7. Enable and Start the UART Listener Service

```bash
sudo systemctl daemon-reload
sudo systemctl enable uart_listener.service
sudo systemctl start uart_listener.service
sudo systemctl status uart_listener.service
```

View logs:

```bash
journalctl -u uart_listener.service -f
```

---

## 8. Create the Heartbeat Sender Service

### 9.1 Create the Script

```bash
sudo nano /usr/local/bin/heartbeat_sender.py
sudo chmod +x /usr/local/bin/heartbeat_sender.py
```

Paste the contents of `heartbeat_sender.py` and save.

---

### 9.2 Create the systemd Service

```bash
sudo nano /etc/systemd/system/heartbeat.service
```

```ini
[Unit]
Description=UART5 Heartbeat Sender
After=network.target multi-user.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/antho/heartbeat_sender.py
Restart=always
WorkingDirectory=/home/antho

[Install]
WantedBy=multi-user.target
```

---

### 9.3 Enable and Start the Service

```bash
sudo systemctl daemon-reload
sudo systemctl enable heartbeat.service
sudo systemctl restart heartbeat.service
```

Check logs:

```bash
journalctl -u heartbeat.service -f
```

---

## 9. File Locations Summary

| Purpose | File Path | Notes |
|-------|-----------|-------|
| UART5 overlay configuration | `/boot/firmware/config.txt` | Must contain `dtoverlay=uart5` |
| UART listener script | `/home/antho/uart5_listener.py` | Must be executable |
| UART listener service | `/etc/systemd/system/uart_listener.service` | Auto-start on boot |
| Heartbeat service | `/etc/systemd/system/heartbeat.service` | Periodic UART TX |
| Logs | `journalctl` | `journalctl -u <service> -f` |

---


## Set auto logging

Create and open a new file

```bash

sudo mkdir -p /etc/systemd/system/getty@tty1.service.d
sudo nano /etc/systemd/system/getty@tty1.service.d/autologin.conf

```

Add the following code

```bash
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin pi --noclear %I $TERM
```
save the file this will enable auto login



## Hide the login Text

```bash
sudo nano /boot/firmware/cmdline.txt
```
Add the following to the end of the line:

quiet loglevel=0 vt.global_cursor_default=0


## Add the splash Screen

copy the splashscreen to the RPI

- move into the folder where the splash screen is to be stored
- make sure that the f copy the file from the location where you original stored the file

 ```bash

cd /opt
sudo mv /home/antho/filename.png  splash.png
sudo chmod +x splash.png

``` 

## add meters

copy you meter configuration into opt/1024x600



## 10. Enable Auto-Login on Local Console

```bash
sudo raspi-config
```

Navigate to:

System Options → Boot / Auto Login

Choose one:

- **B2** Console Autologin
- **B4** Desktop Autologin

Reboot when prompted.

---

## 11. Troubleshooting

### 12.1 UART5 Not Working

Check overlay:

```bash
grep uart5 /boot/firmware/config.txt
```

Check devices:

```bash
ls -l /dev/ttyAMA* /dev/ttyS* 2>/dev/null
```

Ensure no serial console is enabled:

```bash
sudo nano /boot/firmware/cmdline.txt
```

Remove any `console=` entry referencing UART.

---

### 12.2 Service Not Starting

```bash
sudo systemctl status uart_listener.service
```

Reload systemd if needed:

```bash
sudo systemctl daemon-reload
```

---

### 12.3 Permission Denied Accessing UART

```bash
sudo usermod -a -G dialout antho
sudo reboot
```

---

### 12.4 Python `serial` Module Not Found

```bash
sudo apt install python3-serial
```

or

```bash
sudo python3 -m pip install pyserial
```

---
