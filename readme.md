# OpenHantek6022
[![Stability: Active](https://masterminds.github.io/stability/active.svg)](https://masterminds.github.io/stability/active.html)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/OpenHantek/OpenHantek6022)][release]
[![GitHub Release Date](https://img.shields.io/github/release-date/OpenHantek/OpenHantek6022?color=blue)][release]

[![GitHub commits since latest release](https://img.shields.io/github/commits-since/OpenHantek/OpenHantek6022/latest?color=brightgreen)][commits]
[![Build Status](https://travis-ci.org/OpenHantek/OpenHantek6022.svg)](https://travis-ci.org/OpenHantek/OpenHantek6022)
[![Build status](https://ci.appveyor.com/api/projects/status/github/OpenHantek/openhantek6022?svg=true)](https://ci.appveyor.com/project/Ho-Ro/openhantek6022)
[![CodeFactor](https://www.codefactor.io/repository/github/openhantek/openhantek6022/badge)](https://www.codefactor.io/repository/github/openhantek/openhantek6022)

[release]: https://github.com/OpenHantek/OpenHantek6022/releases
[commits]: https://github.com/OpenHantek/OpenHantek6022/commits/master

OpenHantek6022 is a free software for Hantek and compatible (Voltcraft/Darkwire/Protek/Acetech) USB digital signal oscilloscopes. 
It was initially developed by [David Gräff and others](https://github.com/OpenHantek/openhantek/graphs/contributors) on [github.com/OpenHantek/openhantek](https://github.com/OpenHantek/openhantek). 
After David [stopped maintaining](https://github.com/OpenHantek/openhantek/issues/277) the programm I cloned the repo to provide updates - only for Hantek 6022BE/BL.

<p><img alt="Image of main window on linux" width="100%" src="docs/images/screenshot_mainwindow.png"></p>

* Supported devices:
 * Hantek 6022BE and 6022BL as well as compatible scopes (e.g. Voltcraft DSO-2020).
 * SainSmart DDS120 (thx [msiegert](https://github.com/msiegert)) - this device has a different analog front end
 and uses the [slightly improved sigrok firmware](https://github.com/Ho-Ro/sigrok-firmware-fx2lafw), which has [some limitations](https://sigrok.org/wiki/SainSmart_DDS120/Info#Open-source_firmware_details)
 compared to the Hantek scopes (see [#69](https://github.com/OpenHantek/OpenHantek6022/issues/69#issuecomment-607341694)).
* Fully supported operating system: Linux; developed under debian stable for amd64 architecture.
* Raspberry Pi packages (raspbian stable) are available on the [Releases](https://github.com/OpenHantek/OpenHantek6022/releases) page. (For RPI4 see [#28](https://github.com/OpenHantek/OpenHantek6022/issues/28).)
* Compiles under FreeBSD (packaging / installation: work in progress, thx [tspspi](https://github.com/tspspi)).
* Other operating systems builds: [Windows](docs/images/screenshot_mainwindow_win.png) (partly tested) & MacOSX (untested).
* Uses [free open source firmware](https://github.com/Ho-Ro/Hantek6022API), no longer dependent on nonfree Hantek firmware.
* Extensive [User Manual](docs/OpenHantek6022_User_Manual.pdf) with technical specs and schematics.

## Features
* Voltage and Spectrum view for all device supported chanels.
* CH1 and CH2 name becomes red when input is clipped (bottom left).
* Settable probe attenuation factor 1..1000 to accommodate a variety of different probes.
* Measure and display Vpp, RMS, DC (average), AC (rms) and AC as dB values as well as frequency of active channels.
* Math channel modes: CH1+CH2, CH1-CH2, CH2-CH1, CH1*CH2 and AC part of CH1 or CH2.
* Time base 100 ms/div .. 10 ns/div.
* Sample rates 10, 20, 50, 100, 200, 500 kS/s, 1, 2, 5, 10, 12, 15, 24, 30 MS/s (24 & 30 MS/s in CH1-only mode).
* 48 MS/s not supported due to unstable USB data streaming.
* Downsampling (up to 100x) increases solution and SNR.
* Calibration output square wave signal frequency can be selected between 50 Hz .. 100 kHz in 1/2/5 steps.
* Trigger modes: Normal, Auto and Single with green/red status display (top left).
* Calibration values loaded from eeprom or a model configuration file.
* [Calibration program](https://github.com/Ho-Ro/Hantek6022API/blob/master/README.md#create-calibration-values-for-openhantek) to create these values automatically.
* Digital phosphor effect to notice even short spikes; simple eye-diagram display with alternating trigger slope.
* Histogram function for voltage channels on right screen margin.
* A [zoom view](docs/images/screenshot_mainwindow_with_zoom.png) with a freely selectable range.
* Cursor measurement function for voltage, time, amplitude and frequency.
* Export of the graphs to CSV, JPG, PNG file or to the printer.
* Freely configurable colors.
* Automatic adaption of iconset for light and [dark themes](docs/images/screenshot_mainwindow_dark.png).
* The dock views on the main window can be customized by dragging them around and stacking them.
  This allows a minimum window size of 640*480 for old workstation computers.
* All settings can be saved to a configuration file and loaded again.

## AC coupling
* A [little HW modification](docs/HANTEK6022_AC_Modification.pdf) adds AC coupling. OpenHantek6022 supports this feature since v2.17-rc5 / FW0204.

## Install prebuilt binary
* Download Linux, Raspberry Pi, Win and MacOSX packages from the [Releases](https://github.com/OpenHantek/OpenHantek6022/releases) page. (the Win and MacOSX packages are not tested, neither is the installation of the *.rpm packages).
* To install the downloaded *.deb package, open a terminal window, go to the download directory and enter the command (as root) `apt install ./openhantek_..._amd64.deb`.
This command will automatically install all dependencies of the program as well.
* For installation of *.rpm packages follow similar rules, e.g. `dnf install ./openhantek-...-1.x86_64.rpm`.
* Get MacOSX packages from [macports](https://www.macports.org/ports.php?by=name&substr=openhantek) - thx [ra1nb0w](https://github.com/ra1nb0w).
* Get [Fedora rpm packages](https://pkgs.org/download/openhantek) - thx [Vascom](https://github.com/Vascom).
* [Download (untested) Windows build from last commit](https://ci.appveyor.com/project/Ho-Ro/openhantek6022/build/artifacts).

## Building OpenHantek from source
You need the following software, to build OpenHantek from source:
* [CMake 3.5+](https://cmake.org/download/)
* [Qt 5.4+](https://www1.qt.io/download-open-source/)
* [FFTW 3+](http://www.fftw.org/) (prebuild files will be downloaded on windows)
* [libusb-1.0](https://libusb.info/), version >= 1.0.16 (prebuild files will be used on windows)
* A compiler that supports C++11 - tested with gcc, clang and msvc

We have build instructions available for [Linux](docs/build.md#linux), [Apple MacOSX](docs/build.md#macosx) and [Microsoft Windows](docs/build.md#windows).

## Run OpenHantek
On a Linux system start the program via the menu entry *OpenHantek (Digital Storage Oscilloscope)* or from a terminal window as `OpenHantek`.

OpenHantek6022 runs also on legacy HW/SW that supports at least OpenGL 2.1+ or OpenGL ES 1.2+.
OpenGL is preferred, if available. Overwrite this behaviour by starting OpenHantek
from the command line like this: `OpenHantek --useGLES`.

Raspberry Pi uses OpenGL ES automatically.

USB access for the device is required:
* As seen on the [Microsoft Windows build instructions](docs/build.md#windows) page, you have to assign an usb driver to the device.
The original Hantek driver doesn't work.
* On Linux, you need to copy the file `utils/udev_rules/60-hantek.rules` to `/etc/udev/rules.d/` or `/lib/udev/rules.d/` and replug your device. If OpenHantek is installed from a *.deb or *.rpm package this file is installed automatically.

## Important!
The scope doesn't store the firmware permanently in flash or eeprom, it must be uploaded after each power-up and is kept in ram 'til power-down.
If the scope was used with a different software (old openhantek, sigrok or the windows software) the scope must be unplugged and replugged one-time before using it with OpenHantek6022 to enable the automatic loading of the correct firmware.
The top line of the program must display the correct firmware version (FW0204).

## Specifications, features, limitations and developer documentation
I use this project mainly to explore how DSP can improve and extend the [limitations](docs/limitations.md) of this kind of low level hardware. It would have been easy to spend a few bucks more to buy a powerful scope - but it would be much less fun :)
Please refer also to the [developer documentation](openhantek/readme.md) pages.

## Contribute
We welcome any reported Github Issue if you have a problem with this software. Send us a pull request for enhancements and fixes. Some random notes:
   - Read [how to properly contribute to open source projects on GitHub][10].
   - Create a separate branch other than *master* for your changes. It is not possible to directly commit to master on this repository.
   - Write [good commit messages][11].
   - [Sign-off][12] your commits.
   - Use the same [coding style and spacing][13]
     (install clang-format. Use make target: `make format` or execute directly from the openhantek directory: `clang-format -style=file src/*`).
   - Open a [pull request][14] with a clear title and description.
   - Read [Add a new device](docs/adddevice.md) if you want to know how to add a device.
   - We recommend QtCreator as IDE on all platforms. It comes with CMake support, a decent compiler, and Qt out of the box.

[10]: http://gun.io/blog/how-to-github-fork-branch-and-pull-request
[11]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html
[12]: https://github.com/probot/dco/blob/master/README.md
[13]: http://llvm.org/docs/CodingStandards.html
[14]: https://help.github.com/articles/using-pull-requests

## Other DSO open source software
* [Firmware used by OpenHantek and python bindings for 6022BE/BL](https://github.com/Ho-Ro/Hantek6022API)
* [sigrok](http://www.sigrok.org)
* [Software for the Hantek 6022BE/BL (win only)](http://pididu.com/wordpress/basicscope/)

## Other related software
* [HScope for Android](https://hscope.martinloren.com/) A one-channel basic version is available free of charge (with in-app purchases).
