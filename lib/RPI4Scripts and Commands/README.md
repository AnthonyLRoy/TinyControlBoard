# UART5 Listener Setup on Moode (Raspberry Pi)

This guide explains how to enable UART5 on a Raspberry Pi, install required Python serial packages, and configure a `systemd` service to automatically run a UART listener script on boot.

---

## 1. Install Moode Using Raspberry Pi Imager

Use the official Raspberry Pi Imager to flash Moode onto your SD card.  
Once Moode is running, update the system:

```bash
sudo apt update
sudo apt upgrade -y
```

---

## 2. Install Required Packages

```bash
sudo apt install ddcutil
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

### A. Preferred Installation (pip)

```bash
sudo python3 -m pip install pyserial
```

Verify installation:

```bash
python3 -c "import serial; print(serial.__version__)"
```

---

### B. If pip is missing

```bash
sudo apt install python3-pip
sudo python3 -m pip install pyserial
```

---

### C. If you encounter:
`error: externally-managed-environment`

Install the apt-managed version:

```bash
sudo apt install python3-serial
```

---

## 5. Create the UART Listener Systemd Service

Create the service file:

```bash
sudo nano /etc/systemd/system/uart_listener.service
```

Paste:

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

Create the script file:

```bash
nano /home/antho/uart5_listener.py
```

Paste the contents of your `UArt5Listener.py` script.

Set permissions:

```bash
chmod +x /home/antho/uart5_listener.py
sudo chown antho:antho /home/antho/uart5_listener.py
```

---

## 7. Enable and Start the Service

```bash
sudo systemctl daemon-reload
sudo systemctl enable uart5_listener.service
sudo systemctl start uart5_listener.service
sudo systemctl status uart5_listener.service
```

---

## 8. View Logs

```bash
journalctl -u uart5_listener.service -f
```

---

# create the heatbeat sender service


### 0 Copy the code to the directory

create the file 

```bash
/usr/local/bin/heartbeat_sender.py

```
copy the contents from the file heartbeat_sender.py located in this folder and save

### 1 Create the service 

```bash
 sudo nano /etc/systemd/system/heartbeat.service
```

### 2 paste the following code and save

```bash
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

### 3 Enable the service

```bash
  sudo systemctl daemon-reload
  sudo systemctl enable heartbeat.service
  sudo systemctl start heartbeat.service
```
### 4 Check the logs 
 ```bash
  journalctl -u heartbeat.service -f

 ```

# File Locations Summary

| Purpose                          | File Path                                   | Notes                                |
|----------------------------------|---------------------------------------------|--------------------------------------|
| UART5 overlay configuration      | `/boot/firmware/config.txt`                 | Must contain `dtoverlay=uart5`       |
| UART listener Python script      | `/home/antho/uart5_listener.py`             | Must be executable                    |
| Systemd service file             | `/etc/systemd/system/uart_listener.service` | Controls auto-start on boot          |
| Systemd journal logs             | Managed via `journalctl`                    | `journalctl -u uart5_listener.service -f` |
| Python site-packages (pip)       | `/usr/local/lib/python3.x/dist-packages/`   | Version may vary                      |
| Python interpreter used by systemd | `/usr/bin/python3`                         | Ensure correct version                |

---


# Set audo password login on local console

```bash
sudo raspi-config
```

Use the arrow keys to navigate to System Options and press Enter.

Select Boot / Auto Login and press Enter.

Choose the appropriate option for your needs:
  B2 Console Autologin to boot to the command line without requiring a login.
  B4 Desktop Autologin to boot directly into the desktop environment without a login prompt.
Press Enter to confirm your selection.

Use the right arrow key to select and press Enter.

The tool will ask if you want to reboot. Select Yes and press Enter for the changes to take effect. 

After rebooting, the Raspberry Pi will automatically log in with the selected user account. 

# Troubleshooting

### 1. UART5 not working

**Symptoms:**
- No data received  
- UART device missing (`/dev/ttyAMA*` / `/dev/ttyS*`)  
- Serial port busy  

**Fixes:**
Check overlay is active:

```bash
grep uart5 /boot/firmware/config.txt
```

Ensure the UART5 device exists:

```bash
ls -l /dev/ttyAMA* /dev/ttyS* 2>/dev/null
```

Make sure no console is attached to serial:

```bash
sudo nano /boot/firmware/cmdline.txt
```

Ensure no `console=` argument references UART.

---

### 2. Service not starting

Check:

```bash
sudo systemctl status uart5_listener.service
```

**Fixes:**
- Wrong script path → correct `ExecStart`
- Permissions:

  ```bash
  sudo chown antho:antho /home/antho/uart5_listener.py
  sudo chmod +x /home/antho/uart5_listener.py
  ```

- Reload systemd:

  ```bash
  sudo systemctl daemon-reload
  ```

---

### 3. Python import error: `serial` module not found

Install:

```bash
sudo python3 -m pip install pyserial
```

Or:

```bash
sudo apt install python3-serial
```

---

### 4. `externally-managed-environment` pip error

Install pyserial via apt:

```bash
sudo apt install python3-serial
```

---

### 5. Permission denied accessing UART

Check:

```bash
ls -l /dev/ttyAMA*
```

Add user to dialout group:

```bash
sudo usermod -a -G dialout antho
sudo reboot
```

---



