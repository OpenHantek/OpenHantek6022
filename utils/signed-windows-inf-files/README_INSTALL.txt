These files provide an easy method to bind WinUSB to Hantek 6022BE and 6022BL (in scope mode,
button pressed) with clear dstinction between three common states:
- loader mode (when plugged)
- openht mode (when OpenHantek has loaded it's firmware)
- sigrok mode (when Sigrok or PulseView has loaded it's firmware)

When these bindings are used, the device appears under "Device Manager" as 
"Universal Serial Bus devices" with an informative name:
  Hantek 6022BE loader           Hantek 6022BL loader
  Hantek 6022BE openht           Hantek 6022BL openht
  Hantek 6022BE sigrok           Hantek 6022BL sigrok

---
Removing other bindings/drivers (optional, recommended):

The manufacturer's drivers can perhaps be desintalled from "add/remove programs" where they
appear as "Windows Driver Package - ODM (Hantek6022...) USB".

If other bindings appear in "Device Manager" and need to be removed, 
right-click on the device, perform "Uninstall device",
optionally (suggested) tick "Delete the driver software for this device",
confirm.

USBDeview ( https://www.nirsoft.net/utils/usb_devices_view.html ) has an
"Uninstall selected device" option, that does not delete the driver software.

---
To install: Select the desired .inf files, e.g. for a 6022BE:
  Hantek_6022BE_loader.inf  Hantek_6022BE_openht.inf   Hantek_6022BE_sigrok.inf
and right-click to install. This is fast. No reboot or wait is necessary.

Note: Leave the .inf and matching signed .cat files together.

---
Public domin. Use at your own risk.
Only insurance is that these files, as created, are not deliberately malicious.

  fgrieu - 2021-12-23
  Ho-Ro - 2022-05-22 (fixed typos)

---
One user reported that a reboot was required after installation for the device to work properly.
https://github.com/OpenHantek/OpenHantek6022/issues/302
Ok, I've had success. Here are the steps I took:
1 Insert USB
2 Uninstall device and remove drivers
3 Re-insert USB
4 Install Hantek_6022BE_loader.inf
5 Install Hantek_6022BE_openht.inf
6 Restart PC
7 Launch OpenHantek

  Ho-Ro - 2022-05-22
