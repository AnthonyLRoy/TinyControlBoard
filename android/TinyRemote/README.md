# TinyRemote — Android App Setup & Deployment

## Prerequisites

| Requirement | Version |
|---|---|
| Android Studio | Hedgehog (2023.1) or newer |
| Android SDK | API 34 (compileSdk / targetSdk) |
| JDK | 17 (bundled with Android Studio — use Studio's JBR) |
| Android phone | Android 6.0+ (API 23), Bluetooth LE capable |
| TinyControlBoard firmware | `feature/bt-remote` branch flashed |

---

## 1 · Open the Project in Android Studio

1. Launch **Android Studio**.
2. Choose **File → Open** (or *Open* from the Welcome screen).
3. Navigate to and select:
   ```
   d:\Dev\TinyControlBoard\android\TinyRemote
   ```
4. Click **OK** — Studio will detect the Gradle project and sync automatically.
5. Wait for the Gradle sync to finish (progress bar at the bottom). This downloads
   all dependencies (~200 MB on first run).

> **If sync fails with "SDK location not found":** Studio should create
> `local.properties` automatically. If not, create it manually at
> `android/TinyRemote/local.properties` containing:
> ```
> sdk.dir=C\:\\Users\\<your-username>\\AppData\\Local\\Android\\Sdk
> ```

---

## 2 · Add an Android Run Configuration

If the green ▶ Run button is greyed out or no configuration is shown in the
toolbar dropdown, add one manually:

1. Open **Run → Edit Configurations…** (or click the dropdown next to ▶ and
   choose *Edit Configurations*).
2. Click **+** (top-left) → **Android App**.
3. Fill in:

   | Field | Value |
   |---|---|
   | **Name** | `TinyRemote` |
   | **Module** | `TinyRemote.app` |
   | **Launch** | `Default Activity` |
   | **Deployment target** | `Connected Device` (or choose USB device) |

4. Click **OK**.

The toolbar should now show **TinyRemote ▶** ready to run.

---

## 3 · Enable USB Debugging on Your Phone

1. Open **Settings → About phone**.
2. Tap **Build number** seven times to unlock Developer Options.
3. Go to **Settings → Developer Options** → enable **USB Debugging**.
4. Connect the phone via USB cable.
5. Accept the **"Allow USB debugging?"** prompt on the phone.

Verify the device is visible:
```powershell
adb devices
# Should show: <serial>    device
```

---

## 4 · Deploy (Run from Android Studio)

1. Ensure your phone is connected and listed in the device dropdown in the
   Android Studio toolbar.
2. Select the **TinyRemote** configuration.
3. Click the green **▶ Run** button (or press `Shift+F10`).
4. Studio builds, installs, and launches the app automatically.

### Alternative — Command-line install (debug APK)

```powershell
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
$gradle = "$env:USERPROFILE\.gradle\wrapper\dists\gradle-8.7-bin\bhs2wmbdwecv87pi65oeuq5iu\gradle-8.7\bin\gradle.bat"
cd d:\Dev\TinyControlBoard\android\TinyRemote
& $gradle assembleDebug --no-daemon
adb install -r app\build\outputs\apk\debug\app-debug.apk
```

The `-r` flag reinstalls over any existing version without losing data.

---

## 5 · Firmware Prerequisite

The ESP32-S3 must be running the `feature/bt-remote` firmware. To flash it:

1. Open the workspace in VS Code.
2. Run the **PlatformIO: Upload** task (Terminal → Run Task → *PlatformIO Upload*),
   or use the PlatformIO toolbar ➜ button.
3. The serial monitor (115200 baud) should show:
   ```
   I BLE_Server: BLE advertising as "TinyControlBoard"
   ```

---

## 6 · First-Run Walkthrough

### Permissions
On first launch the app requests Bluetooth permissions:

- **Android 12+** — grants `BLUETOOTH_SCAN` + `BLUETOOTH_CONNECT`. Tap **Allow**.
- **Android 6–11** — grants `BLUETOOTH` + `ACCESS_FINE_LOCATION`. Tap **Allow**.

Location permission on older Android is required by the OS for BLE scanning; the
app never uses your actual location.

### Connecting
1. Make sure Bluetooth is **on**.
2. Tap the amber **Scan** button.  
   The spinning indicator starts; found devices appear in the list.
3. Tap **TinyControlBoard** in the list.  
   The app connects and opens the **Control Panel**.

> If the device doesn't appear after 10 s, verify the firmware is running and
> the board is powered on. The board advertises continuously and re-advertises
> after every disconnect.

### Control Panel
- **Power state** is shown at the top (OFF / ON / SLEEP / …).
- Each of the 16 buttons mirrors a physical button on the board.
- **Amber LED dots** in the top-right of each card reflect the physical LED state
  (updated automatically every ~500 ms via BLE notification).
- Tap any button to trigger the corresponding command on the board.
- Navigating back disconnects from the board and returns to the Scan screen.

---

## 7 · BLE Technical Reference

| Item | Value |
|---|---|
| Device name | `TinyControlBoard` |
| Service UUID | `4a5c6e7d-8f9a-4b2c-1d3e-5f6a7b8c9d0e` |
| Command char (WRITE_NO_RESPONSE) | `4a5c6e7d-…-9d01` — 2-byte LE uint16 CommandId |
| Status char (READ \| NOTIFY) | `4a5c6e7d-…-9d02` — 3 bytes: `[powerState, bitmask_lo, bitmask_hi]` |
| Status push interval | 500 ms (only on change) |

### Power state byte values
| Value | State |
|---|---|
| 0 | OFF |
| 1 | SHUTTING DOWN |
| 2 | ON |
| 3 | TURNING ON |
| 4 | SLEEP |
| 5 | GOING TO SLEEP |
| 6 | DEEP SLEEP |
| 7 | GOING INTO DEEP SLEEP |

---

## 8 · Troubleshooting

| Symptom | Fix |
|---|---|
| Scan finds nothing | Verify board is powered and firmware is `feature/bt-remote`. Check BLE is on. |
| "Bluetooth permissions required" toast | Open phone Settings → Apps → TinyRemote → Permissions and grant Bluetooth (and Location on Android <12). |
| Gradle sync fails — "Could not resolve…" | Check internet connection; run `gradle --refresh-dependencies`. |
| `adb devices` shows no device | Reconnect USB, accept debug prompt on phone, check USB mode is "File transfer" not "Charging only". |
| Commands sent but board doesn't respond | Confirm the power state is **ON** (board ignores most commands when in SLEEP or OFF). |
| App disconnects immediately after connect | BLE notify subscription may have failed. Force-stop the app, reopen, and reconnect. |
