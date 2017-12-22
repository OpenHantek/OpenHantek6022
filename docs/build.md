---
layout: default
---
### [Linux](#linux)
For debian based systems (Ubuntu, Mint) install named requirements like this:
> apt install g++ cmake qttools5-dev qttools5-dev-tools libfftw3-dev binutils-dev libusb-1.0-0-dev libqt5opengl5-dev

For rpm based distributions (Fedora) use this command:
> dnf install cmake gcc-c++ qt5-qtbase-gui qt5-qttools-devel qt5-qttranslations fftw-devel binutils-devel libusb-devel

After you've installed the requirements run the following commands inside the directory of this package:
> mkdir build <br>
> cd build <br>
> cmake ../ <br>
> make -j

Optionally install the program:

> sudo make install

If you do not install the program, you need to copy the file `firmware/60-hantek.rules` to `/lib/udev/rules.d/`,
and reload the udev service, otherwise you will not have the correct permissions to access usb devices.

### [Apple MacOSX](#apple)
We recommend homebrew to install the required libraries.
> brew update <br>
> brew install libusb fftw qt5;

After you've installed the requirements run the following commands inside the directory of this package:
> mkdir build <br>
> cd build <br>
> cmake ../ -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.7.0 <br>
> make -j

Please adjust the path to Qt5. You can find the path with the command:
> brew info qt5

### [Windows](#windows)

Run the **CMake GUI** program and select the source directory, build directory and your compiler.
If your compiler is for example Visual Studio, cmake will generate a Visual Studio Project and solution file (\*.sln).
Open the project and build it. If you use an IDE that has inbuild support for cmake projects like QtCreator,
you can just open and build this project.

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

