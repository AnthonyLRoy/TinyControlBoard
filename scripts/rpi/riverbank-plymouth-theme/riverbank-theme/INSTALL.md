# RiverBank Plymouth Splash - Install Instructions

## 1. Copy this folder to the Pi
From your computer:
```
scp -r riverbank-theme pi@<moode-ip>:/tmp/
```
(replace `pi` and `<moode-ip>` with your actual username/IP)

## 2. Install prerequisites (if not already done)
```
sudo apt update
sudo apt install plymouth plymouth-themes
```

## 3. Move the theme into place
```
sudo mkdir -p /usr/share/plymouth/themes/riverbank
sudo cp /tmp/riverbank-theme/background.png /usr/share/plymouth/themes/riverbank/
sudo cp /tmp/riverbank-theme/spinner.png /usr/share/plymouth/themes/riverbank/
sudo cp /tmp/riverbank-theme/riverbank.plymouth /usr/share/plymouth/themes/riverbank/
sudo cp /tmp/riverbank-theme/riverbank.script /usr/share/plymouth/themes/riverbank/
```

## 4. Enable splash mode in the boot config
Edit `/boot/firmware/cmdline.txt` (single line file - add to the end of the
existing line, don't create a new line):
```
splash quiet plymouth.ignore-serial-consoles
```

## 5. Set the theme and rebuild the initramfs
```
sudo plymouth-set-default-theme -R riverbank
```
(the `-R` flag rebuilds the initramfs for you; if your Plymouth version
doesn't support `-R`, run `sudo plymouth-set-default-theme riverbank`
followed by `sudo update-initramfs -u`)

## 6. Reboot and check
```
sudo reboot
```
You should see the RiverBank banner full-screen (letterboxed to fit your
display, not stretched or cropped) with a small blue spinner ring rotating
near the bottom, until Moode's own services/UI take over.

## Notes
- The background never moves or spins - only the small ring icon animates,
  so it reads as "a waiting indicator on top of a static banner," not a
  spinning image.
- If you have the known Raspberry Pi OS Trixie plymouth graphics-theme
  regression, you may still see 3 flashing dots instead of this - test
  `sudo plymouth --show-splash` isn't reliable for previewing this over SSH;
  the real test is a physical reboot with a display attached.
- To preview/tweak without rebooting every time, you can run (on the Pi,
  from a real console, not SSH):
  ```
  sudo plymouthd --debug --no-daemon &
  sudo plymouth --show-splash
  ```
  then `sudo plymouth --quit` to end the preview.
