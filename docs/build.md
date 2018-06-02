---
layout: default
---
### [Linux](#linux)
For debian (stretch and newer), Ubuntu 17.04+ and Mint 17+ and other deb based distributions install named requirements like this:
> apt install g++ cmake qttools5-dev qttools5-dev-tools libfftw3-dev binutils-dev libusb-1.0-0-dev libqt5opengl5-dev mesa-common-dev libgl1-mesa-dev libgles2-mesa-dev

For distributions using dnf package manager (Fedora 21+) use this command:
> dnf install cmake gcc-c++ qt5-qtbase-gui qt5-qttools-devel qt5-qttranslations fftw-devel binutils-devel libusb-devel mesa-libGL-devel mesa-libGLES-devel

For OpenSUSE and related distributions use this command
zypper install cmake gcc-c++ qt5-qtbase-devel qt5-qttools-devel qt5-qttranslations  binutils-devel libusb-devel Mesa-libGL-devel Mesa-libGLESv2-devel fftw3-devel 

After you've installed the requirements run the following commands inside the directory of this package:
> mkdir build <br>
> cd build <br>
> cmake ../ <br>
> make -j2

Optionally install the program:

> sudo make install

If you do not install the program, you need to copy the file `firmware/60-hantek.rules` to `/lib/udev/rules.d/` yourself,
and replug your device, otherwise you will not have the correct permissions to access usb devices.

### [Apple MacOSX](#apple)
We recommend homebrew to install the required libraries.
> brew update <br>
> brew install libusb fftw qt5 cmake;

After you've installed the requirements run the following commands inside the directory of this package:
> mkdir build <br>
> cd build <br>
> cmake ../ -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt/5.10.1 <br>
> make -j2

Please adjust the path to Qt5. You can find the path with the command:
> brew info qt5

### [Windows](#windows)

We highly recommend to use QtCreator to build this software. All reported issues regarding other IDEs
will be closed as invalid!

* Open the project in QtCreator
* Compile the software

Hints for Visual Studio 2015/2017 users:
* Install the right Qt package that matches your Visual Studio installation.
* Build for 64bit. 32bit builds theoretically work, but you are on your own then.
* Use the **CMake GUI** to setup all required Qt include and library paths.

Microsoft Windows needs an installed driver for every usb device:

* Make sure your original Hantek driver is uninstalled.
* Extract `cmake/winusb driver.zip` and customize the `libusb_device.inf` file for your device. The Vendor ID and Device ID as well as a unique GUID need to be entered like in the following example for a Hantek 6022BE.
* Physically plug (or replug) oscilloscope into PC's. From the Windows device manager update driver for your device and point to your modified libusb_device.inf.

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

