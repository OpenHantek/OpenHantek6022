---
layout: default
---
### [Linux](#linux)
For debian (stretch and newer), Ubuntu 17.04+ and Mint 17+ and other deb based distributions install named requirements like this:
> apt install g++ make cmake qttools5-dev qttools5-dev-tools libfftw3-dev binutils-dev libusb-1.0-0-dev libqt5opengl5-dev mesa-common-dev libgl1-mesa-dev libgles2-mesa-dev

For distributions using dnf package manager (Fedora 21+) use this command:
> dnf install make cmake gcc-c++ qt5-qtbase-gui qt5-qttools-devel qt5-qttranslations fftw-devel binutils-devel libusb-devel mesa-libGL-devel mesa-libGLES-devel

For OpenSUSE and related distributions use this command
> zypper install make cmake gcc-c++ qt5-qtbase-devel qt5-qttools-devel qt5-qttranslations  binutils-devel libusb-devel Mesa-libGL-devel Mesa-libGLESv2-devel fftw3-devel 

After you've installed the requirements run the following commands inside the directory of this package:
> mkdir build <br>
> cd build <br>
> cmake ../ <br>
> make -j2

Optionally install the program:

> sudo make install

Optionally create a debian package:

> sudo make package

If you do not install the program, you need to copy the file `firmware/60-hantek.rules` to `/lib/udev/rules.d/` yourself,
and replug your device, otherwise you will not have the correct permissions to access usb devices.

### [MacOSX](#macosx)
We recommend homebrew to install the required libraries.

    brew update
    brew install libusb fftw qt5 cmake

If you want to build an OSX bundle make sure the option in `openhantek/CMakeLists.txt` is set accordingly:

`option(BUILD_MACOSX_BUNDLE "Build MacOS app bundle" ON)`

After you've installed the requirements run the following commands inside the top directory of this package:

    mkdir build
    cd build
    cmake ..
    #
    make -j2
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
