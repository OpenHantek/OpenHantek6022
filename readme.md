# OpenHantek [![Build Status](https://travis-ci.org/OpenHantek/openhantek.svg?branch=master)](https://travis-ci.org/OpenHantek/openhantek) [![Build status](https://ci.appveyor.com/api/projects/status/9w4rd5r04ufqafr4/branch/master?svg=true)](https://ci.appveyor.com/project/davidgraeff/openhantek/branch/master)
OpenHantek is a free software for Hantek (Voltcraft/Darkwire/Protek/Acetech) USB digital storage oscilloscopes based on HantekDSO and has started as an alternative to the official Hantek DSO software.

Supported operating systems:
* Linux
* MacOSX
* Windows¹

<table><tr>
    <td> <img alt="Image of main window on linux" width="100%" src="doc/screenshot_mainwindow.png"> </td>
    <td> <img alt="Image of main window on Windows" width="100%" src="doc/screenshot_mainwindow_win.png"> </td>
</tr></table>

Supported hantek devices:
* DSO2xxx Series
* DSO52xx Series
* 6022BE/BL

## Install prebuilt binary
Navigate to the [Releases](https://github.com/OpenHantek/openhantek/releases) page 

## Building OpenHantek from source
You need the following packages, to build OpenHantek from source:
* CMake 3.5+
* Qt 5.4+
* FFTW 3+ (prebuild files will be downloaded on windows)
* libusb 1.x (prebuild files will be used on windows)

You need a OpenGL 3.x capable graphics card for OpenHantek.

### Linux
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

### Apple MacOSX
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

### Windows

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

## Special case: 6022BE
You can adjust samplerate and use software triggering for 6022BE.
   - Support 48, 24, 16, 8, 4, 2, 1 M and 500, 200, 100 k Hz samplerates with modded firmware by [jhoenicke](https://github.com/rpcope1/Hantek6022API) 
   - Support software trigger by detecting rising or falling edge of signal.
     Use software trigger item on the trigger menu. For trigger level, adjust left arrow on the right-side of the graph.
   - Note that the first few samples are dropped due to unstable/unusual reading.

## Contribute
Please use Github Issues to report any problems or enhancements or send us pull requests. Some random notes:
   - Read [how to properly contribute to open source projects on GitHub][10].
   - Create a separate branch other than *master* to easily amend changes to a pull request later, if necessary.
   - Write [good commit messages][11].
   - Use the same [coding style and spacing][13]
     (install clang-format. Use make target: `make format` or execute directly from the openhantek directory: `clang-format -style=file src/*`).
   - Open a [pull request][12] with a clear title and description
     
[10]: http://gun.io/blog/how-to-github-fork-branch-and-pull-request
[11]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html
[12]: https://help.github.com/articles/using-pull-requests
[13]: http://llvm.org/docs/CodingStandards.html

## Other open source software
* [SigRok](http://www.sigrok.org)
* [Software for the Hantek 6022BE/BL only](http://pididu.com/wordpress/basicscope/)
