---
layout: default
---
### [Linux](#linux)
For Debian (stretch and newer), Ubuntu 17.04+ and Mint 17+ and other deb based distributions install named requirements like this:
> apt install g++ make cmake qttools5-dev libfftw3-dev binutils-dev libusb-1.0-0-dev libqt5opengl5-dev mesa-common-dev libgl1-mesa-dev libgles2-mesa-dev

For distributions using dnf package manager (Fedora 21+) use this command:
> dnf install make cmake gcc-c++ qt5-qtbase-gui qt5-qttools-devel qt5-qttranslations fftw-devel binutils-devel libusb-devel mesa-libGL-devel mesa-libGLES-devel

For OpenSUSE and related distributions use this command
> zypper install make cmake gcc-c++ libqt5-qtbase libqt5-qttools libqt5-qttranslations libusb-1_0 Mesa-libGL1 Mesa-libGLESv2 fftw3 

After you've installed the requirements run the following commands inside the directory of this package:

    mkdir build
    cd build
    cmake ..
    make -j4

After success you can test the newly built program `openhantek/OpenHantek`.
Due to the included debug information this file is quite big (~20 MB), but the size can be reduced with `strip openhantek/OpenHantek` if you want to put it into a user directory. 
If you do not install the program, you need to copy the file `utils/udev_rules/60-hantek.rules` to `/lib/udev/rules.d/` yourself,
and replug your device, otherwise you will not have the correct USB permissions to access the device.

You can install the program with `sudo make install`, but it is highly recommended to create a debian package,
which allows a clear installation and removal of the package:

    rm -f packages/*
    fakeroot make package
    sudo apt install packages/openhantek_*_amd64.deb

If you detect that icons are not displayed correctly, please check if the Qt SVG library is installed on your system.
The Linux systems mentioned above include this lib when you install according to the provided lists.
However, an [alpine linux](https://alpinelinux.org/) user [reported](https://github.com/OpenHantek/OpenHantek6022/issues/42#issuecomment-564329632) that he had to install `qt5-qtsvg` separately.

----

### [RaspberryPi](#raspberrypi)
The general Linux requirements from above also apply to the RPi; precompiled packages are available as [release](https://github.com/OpenHantek/OpenHantek6022/releases) assets.
Please note, it is important that the correct graphics driver is selected,
the OpenGL implementation of Qt requires the `Original non-GL desktop driver`, e.g. on my *RPi3B+*:

    sudo raspi-config

![Screenshot_20200429_110134](https://user-images.githubusercontent.com/12542359/80594903-22788180-8a24-11ea-9859-eebd51542823.png)

![Screenshot_20200429_110204](https://user-images.githubusercontent.com/12542359/80594920-2c9a8000-8a24-11ea-8629-9584cfaf367f.png)

![Screenshot_20200429_110341](https://user-images.githubusercontent.com/12542359/80594963-3cb25f80-8a24-11ea-9d5a-8ca90e836581.png)

Only the 1st setting `G1 Legacy        Original non-GL desktop driver` worked for `OpenHantek6022`, the other two resulted in an error as below:

    QEGLPlatformContext: eglMakeCurrent failed: 3009
    QOpenGLFunctions created with non-current context

Setting `Original non-GL desktop driver` was reported to work also on *RPi4B+*.

----

### [FreeBSD](#freebsd)
Install the build requirements

    pkg install cmake qt5 fftw3 linux_libusb

After you've installed the requirements run the following commands inside the directory of this package:

    mkdir build
    cd build
    cmake ..
    make -j4

After success you can test the newly built program `openhantek/OpenHantek`.
Due to the included debug information this file is quite big (~20 MB), but the size can be reduced with `strip openhantek/OpenHantek` if you want to put it into a user directory. 

In order for OpenHantek to work, make sure that your USB device has permissions for your user.
You can achieve this by copying [`utils/devd_rules_freebsd/openhantek.conf`](../utils/devd_rules_freebsd/openhantek.conf)
to `/usr/local/etc/devd/`, or create a file with similar content for your device:

    ...

    # Hantek DSO-6022BE

    notify 100 {
        match  "system"     "USB";
        match  "subsystem"  "DEVICE";
        match  "type"       "ATTACH";
        match  "vendor"     "0x04b4";
        match  "product"    "0x6022";
        action "chgrp openhantek /dev/$ugen; chmod g+rw /dev/$ugen; chgrp -h openhantek /dev/$ugen; chmod -h g+rw /dev/$ugen";
    };

    notify 100 {
        match  "system"     "USB";
        match  "subsystem"  "DEVICE";
        match  "type"       "ATTACH";
        match  "vendor"     "0x04b5";
        match  "product"    "0x6022";
        action "chgrp openhantek /dev/$ugen; chmod g+rw /dev/$ugen; chgrp -h openhantek /dev/$ugen; chmod -h g+rw /dev/$ugen";
    };

    ...

- The "action" above doesn't use $device-name due to:
https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=242111

Also please note that devices like this have two vendor/product id combinations,
before and after loading the firmware, hence two commands above.
The action changes the device permissions for supported scope devices:

`rw------- root operator` becomes `rw-rw---- root openhantek`

Make sure to be member of the group `openhantek`, e.g.:

    pw groupadd openhantek -g 6022
    pw groupmod openhantek -M <YOUR_USER>

----

### [MacOSX](#macosx)
We recommend homebrew to install the required libraries.

    brew update
    brew install libusb fftw qt5 cmake

If you want to build an OSX bundle make sure the option in `openhantek/CMakeLists.txt` is set accordingly:

    option(BUILD_MACOSX_BUNDLE "Build MacOS app bundle" ON)

After you've installed the requirements run the following commands inside the top directory of this package:

    mkdir build
    cd build
    cmake ..
    #
    make -j4
    #
    # now the target was created in subdir openhantek
    # .. either as single binary OpenHantek, then you're done
    # .. or as a bundle if enabled in ../../openhantek/CMakeLists.txt
    # .. but this bundle is still a template as the dynlibs are not yet bundled
    # .. this magic will happen now
    #
    cd openhantek
    #
    # deploy all necessary Qt dynlibs into the bundle
    macdeployqt OpenHantek.app -always-overwrite -verbose=2
    #
    # find all other dependencies, and their dependencies, and their... (you got it!)
    python ../../macdeployqtfix/macdeployqtfix.py OpenHantek.app/Contents/MacOS/OpenHantek $(brew --prefix qt5)
    #
    # finally create OpenHantek.dmg from OpenHantek.app
    macdeployqt OpenHantek.app -dmg -no-plugins -verbose=2
    #

----

### [Windows](#windows)

We highly recommend to use QtCreator to build this software. All reported issues regarding other IDEs
will be closed as invalid!

* Open the project in QtCreator
* Compile the software

Hints for Visual Studio 2015/2017 users:
* Install the right Qt package that matches your Visual Studio installation.
* Build for 64bit. 32bit builds theoretically work, but you are on your own then.
* Use the **CMake GUI** to setup all required Qt include and library paths.

#### Microsoft Windows USB driver install (with Zadig)

The device specific USB driver shipped with the vendor software is not going to work in almost all cases. 
You will need to install the WinUSB driver.

For installing the WinUSB driver you can use the [Zadig](http://zadig.akeo.ie/) executable. 
There are two versions, one for Windows XP (zadig_xp.exe), and another one for all other (Vista or higher)
supported Windows versions (zadig.exe). Both 32 and 64 bit Windows versions are supported. 

If you already installed the vendor driver previously, you need to run Zadig and switch to the WinUSB driver (see above). 
There's no need to uninstall or deactivate the vendor driver manually, Zadig will handle all of this.

Note: For Hantek 6022BE and 60222BL you have to assign the WinUSB driver via Zadig twice: 
the first time for the initial USB VID/PID the device has when attaching it via USB, 
and a second time after the firmware has been uploaded to the device and the device has "renumerated" 
with a different VID/PID pair.

See also the [Zadig wiki page](https://github.com/pbatard/libwdi/wiki/Zadig) for more information.

  - 1st install for the newly plugged scope without firmware (VID/PID 04B4/6022 for 6022BE or VID/PID 04B4/602A for 6022BL). 
  - 2nd time for the scope with firmware uploaded (VID/PID 04B5/6022 for 6022BE or VID/PID 04B5/602A for 6022BL).

Some win user reports:

* black2279's wiki entry 
[USB Drivers Installation with Zadig for Hantek 6022 (Windows)](https://github.com/black2279/OpenHantek6022/wiki/USB-Drivers-Installation-with-Zadig-for-Hantek-6022-%28Windows%29)
* raxis13's [success report](https://www.eevblog.com/forum/testgear/hantek-6022be-20mhz-usb-dso/msg2563869/#msg2563869)

----

#### This is the old and more complex procedure (no positive feedback known)
  - Make sure your original Hantek driver is uninstalled.
  - Extract `cmake/winusb driver.zip` and customize the `libusb_device.inf` file for your device. The Vendor ID and Device ID as well as a unique GUID need to be entered like in the following example for a Hantek 6022BE.
  - Physically plug (or replug) oscilloscope into PC's. From the Windows device manager update driver for your device and point to your modified libusb_device.inf.

````
; =====================================================
; ========= START USER CONFIGURABLE SECTION ===========
; =====================================================

DeviceName = "HantekDSO6022BE"
VendorID = "VID_04B5"
ProductID = "PID_6022"
DeviceClassGUID = "{78a1c341-4539-11d3-b88d-00c04fad5171}"
; Date MUST be in MM/DD/YYYY format
Date = "08/12/2017"

; =====================================================
; ========== END USER CONFIGURABLE SECTION ============
; =====================================================
````
