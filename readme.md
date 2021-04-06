# OpenHantek6022
[![Stability: Active](https://masterminds.github.io/stability/active.svg)](https://masterminds.github.io/stability/active.html)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/OpenHantek/OpenHantek6022)][release]
[![GitHub Release Date](https://img.shields.io/github/release-date/OpenHantek/OpenHantek6022?color=blue)][release]

[![GitHub commits since latest release](https://img.shields.io/github/commits-since/OpenHantek/OpenHantek6022/latest?color=brightgreen)][commits]
[![Appveyor CI](https://ci.appveyor.com/api/projects/status/github/OpenHantek/openhantek6022?svg=true)](https://ci.appveyor.com/project/Ho-Ro/openhantek6022)
[![GitHub CI](https://github.com/OpenHantek/OpenHantek6022/actions/workflows/linux_build.yml/badge.svg)](https://github.com/OpenHantek/OpenHantek6022/actions/workflows/linux_build.yml)
[![CodeFactor](https://www.codefactor.io/repository/github/openhantek/openhantek6022/badge)](https://www.codefactor.io/repository/github/openhantek/openhantek6022)

[release]: https://github.com/OpenHantek/OpenHantek6022/releases
[commits]: https://github.com/OpenHantek/OpenHantek6022/commits/master

OpenHantek6022 is a free software for **Hantek DSO6022** USB digital signal oscilloscopes that is actively developed on
[github.com/OpenHantek/OpenHantek6022](https://github.com/OpenHantek/OpenHantek6022) - but only for Hantek 6022BE/BL and compatible scopes (Voltcraft, Darkwire, Protek, Acetech, etc.).

The program was initially developed by [David Gräff and others](https://github.com/OpenHantek/openhantek/graphs/contributors)
on [github.com/OpenHantek/openhantek](https://github.com/OpenHantek/openhantek),
but David [stopped maintaining](https://github.com/OpenHantek/openhantek/issues/277) the programm in December 2018. 

<p><img alt="Image of main window on linux" width="100%" src="docs/images/screenshot_mainwindow.png"></p>

* Supported devices:
 * Hantek 6022BE and 6022BL as well as compatible scopes (e.g. Voltcraft DSO-2020).
 * SainSmart DDS120 (thx [msiegert](https://github.com/msiegert)) - this device has a different analog front end
 and uses the [slightly improved sigrok firmware](https://github.com/Ho-Ro/sigrok-firmware-fx2lafw), which has [some limitations](https://sigrok.org/wiki/SainSmart_DDS120/Info#Open-source_firmware_details)
 compared to the Hantek scopes (see [#69](https://github.com/OpenHantek/OpenHantek6022/issues/69#issuecomment-607341694)).
* Demo mode is provided by the `-d` or `--demoMode` command line option.
* Fully supported operating system: Linux; developed under debian stable for amd64 architecture.
* Raspberry Pi packages (raspbian stable) are available on the [Releases](https://github.com/OpenHantek/OpenHantek6022/releases) page, check this [setup requirement](docs/build.md#raspberrypi).
* Compiles under FreeBSD (packaging / installation: work in progress, thx [tspspi](https://github.com/tspspi)).
* Other operating systems builds: [Windows](docs/images/screenshot_mainwindow_win.png) (mostly untested) & macOS (completely untested).

 **No support for non-Linux related issues unless a volunteer steps in!**
* Uses [free open source firmware](https://github.com/Ho-Ro/Hantek6022API), no longer dependent on nonfree Hantek firmware.
* Extensive [User Manual](docs/OpenHantek6022_User_Manual.pdf) with technical specs and schematics.

## Features
* Voltage and Spectrum view for all device supported chanels.
* CH1 and CH2 name becomes red when input is clipped (bottom left).
* Settable probe attenuation factor 1..1000 to accommodate a variety of different probes.
* Measure and display Vpp, DC (average), AC, RMS and dB (of RMS) values as well as frequency of active channels.
* Display the power dissipation for a load resistance of 1..1000 Ω (optional, can be set in Oscilloscope/Settings/Analysis).
* Display the THD of the signal (optional, can be set in Oscilloscope/Settings/Analysis).
* Math channel modes: CH1+CH2, CH1-CH2, CH2-CH1, CH1*CH2 and AC part of CH1 or CH2.
* Time base 10 ns/div .. 10 s/div.
* Sample rates 100, 200, 500 S/s, 1, 2, 5, 10, 20, 50, 100, 200, 500 kS/s, 1, 2, 5, 10, 12, 15, 24, 30 MS/s (24 & 30 MS/s in CH1-only mode).
* 48 MS/s not supported due to unstable USB data streaming.
* Downsampling (up to 200x) increases resolution and SNR.
* Calibration output square wave signal frequency can be selected between 32 Hz .. 100 kHz in small steps (poor man's signal generator).
* Trigger modes: *Normal*, *Auto* and *Single* with green/red status display (top left).
* Untriggered *Roll* mode can be selected for slow time bases of 200 ms/div .. 10 s/div.
* Trigger filter *HF* (trigger also on glitches), *Normal* and *LF* (for noisy signals).
* Display interpolation modes *Off*, *Linear*, *Step* and *Sinc*.
* Calibration values loaded from eeprom or a model configuration file.
* [Calibration program](https://github.com/Ho-Ro/Hantek6022API/blob/master/README.md#create-calibration-values-for-openhantek) to create these values automatically.
* Digital phosphor effect to notice even short spikes; simple eye-diagram display with alternating trigger slope.
* Histogram function for voltage channels on right screen margin.
* A [zoom view](docs/images/screenshot_mainwindow_with_zoom.png) with a freely selectable range.
* Cursor measurement function for voltage, time, amplitude and frequency.
* Export of the graphs to CSV, JPG, PNG file or to the printer.
* Freely configurable colors.
* Automatic adaption of iconset for light and [dark themes](docs/images/screenshot_mainwindow_dark.png).
* The dock views on the main window can be [customized](https://github.com/OpenHantek/OpenHantek6022/issues/161#issuecomment-799597664) by dragging them around and stacking them.
  This allows a minimum window size of 800*300 for old laptops or workstation computers.
* All settings can be saved to a configuration file and loaded again.
* French, German, Russian and Spanish localisation complete, Italian and Portuguese translation ongoing.

## AC coupling
* A [little HW modification](docs/HANTEK6022_AC_Modification.pdf) adds AC coupling. OpenHantek6022 supports this feature since v2.17-rc5 / FW0204.

## Building OpenHantek from source
You need the following software, to build OpenHantek from source:
* [CMake 3.5+](https://cmake.org/download/)
* [Qt 5.4+](https://www1.qt.io/download-open-source/)
* [FFTW 3+](http://www.fftw.org/) (prebuild files will be downloaded on windows)
* [libusb-1.0](https://libusb.info/), version >= 1.0.16 (prebuild files will be used on windows)
* A compiler that supports C++11 - tested with gcc, clang and msvc

We have build instructions available for [Linux](docs/build.md#linux), [Raspberry Pi](docs/build.md#raspberrypi), [FreeBSD](docs/build.md#freebsd), [Apple macOS](docs/build.md#macos) and [Microsoft Windows](docs/build.md#windows).
To make building for Linux even easier, I provide two shell scripts:
* [`LinuxSetup_AsRoot`](LinuxSetup_AsRoot), which installs all build requirements. You only need to call this script once (as root) if you have cloned the project.
* [`LinuxBuild`](LinuxBuild) configures the build, builds the binary and finally creates the packages (deb, rpm and tgz) that can be installed as described in the next paragraph.
If you make small changes to the local source code, it is sufficient to call `make -j4` or `fakeroot make -j4 package` in the `build` directory.

## Install prebuilt binary packages
* Download Linux, Raspberry Pi, macOS and Windows packages from the [Releases](https://github.com/OpenHantek/OpenHantek6022/releases) page.
(The RPi, macOS and Windows packages are not tested, neither is the installation of the `*.rpm` packages. For RPi4 see also [issue #28](https://github.com/OpenHantek/OpenHantek6022/issues/28).)
* If you want to follow ongoing development, packages built from the last commit are available in the [unstable release](https://github.com/OpenHantek/OpenHantek6022/releases/tag/unstable).
* As I develop on a *Debian stable* system the preferred (native) package format is `*.deb`.
* To install the downloaded `*.deb` package, open a terminal window, go to the package directory and enter the command (as root) `apt install ./openhantek_..._amd64.deb`.
This command will automatically install all dependencies of the program as well.
* For installation of `*.rpm` packages follow similar rules, e.g. `dnf install ./openhantek-...-1.x86_64.rpm`.
* The `*.tar.gz` achives contain the same files as the `*.deb` and `*.rpm` packages for quick testing - do not use for a permanent intallation. Do not report any issues about the `*.tar.gz`!
* Get macOS packages from [macports](https://www.macports.org/ports.php?by=name&substr=openhantek) - thx [ra1nb0w](https://github.com/ra1nb0w).
* Get [Fedora rpm packages](https://pkgs.org/download/openhantek) - thx [Vascom](https://github.com/Vascom).
* Download [(untested) builds from last commit](https://ci.appveyor.com/project/Ho-Ro/openhantek6022). Select the preferred `Image` and go to `Artifacts`.

## Run OpenHantek6022
On a Linux system start the program via the menu entry *OpenHantek (Digital Storage Oscilloscope)* or from a terminal window as `OpenHantek`.

You can explore the look and feel of OpenHantek6022 without the need for real scope hardware by running it from the command line as: `OpenHantek --demoMode`.

OpenHantek6022 runs also on legacy HW/SW that supports at least *OpenGL* 2.1+ or *OpenGL ES* 1.2+.
OpenGL is preferred, select *OpenGL ES* by starting OpenHantek
from the command line like this: `OpenHantek --useGLES`.

The Raspberry Pi build uses OpenGL ES automatically, check also the [graphics driver setup](docs/build.md#raspberrypi).

USB access for the device is required (unless using demo mode):
* On Linux, you need to copy the file `utils/udev_rules/60-openhantek.rules` to `/etc/udev/rules.d/` or `/usr/lib/udev/rules.d/` and replug your device.
If OpenHantek is installed from a *.deb or *.rpm package this file is installed automatically into `/usr/lib/udev/rules.d/`.

### Windows USB access
* **The original Hantek driver for Windows doesn't work!** You have to assign the correct WinUSB driver with the tool [Zadig](https://zadig.akeo.ie/). 
Check the [Microsoft Windows build instructions](docs/build.md#microsoft-windows-usb-driver-install-with-zadig) and follow the good [step-by-step tutorial](docs/OpenHantek6022_zadig_Win10.pdf) provided by [DaPa](https://github.com/DaPa).

## Important!
The scope doesn't store the firmware permanently in flash or eeprom, it must be uploaded after each power-up and is kept in ram 'til power-down.
If the scope was used with a different software (old openhantek, sigrok or the windows software) the scope must be unplugged and replugged one-time before using it with OpenHantek6022 to enable the automatic loading of the correct firmware.
The top line of the program must display the correct firmware version (FW0209).

## Specifications, features, limitations and developer documentation
I use this project mainly to explore how DSP software can improve and extend the [limitations](docs/limitations.md)
of this kind of low level hardware. It would have been easy to spend a few bucks more to buy a powerful scope - but it would be much less fun :)
Please refer also to the [developer info](openhantek/developer_info.md).

## Contribute
We welcome any reported GitHub issue if you have a problem with this software. Send us a pull request for enhancements and fixes. Some random notes:
   - Read [how to properly contribute to open source projects on GitHub][10].
   - Create a separate branch other than *master* for your changes. It is not possible to directly commit to master on this repository.
   - Write [good commit messages][11].
   - Use the same [coding style and spacing][13] -> install clang-format and use make target: `make format` or execute directly: `clang-format -style=file -i *.cpp *.h`.
   - It is mandatory that your commits are [Signed-off-by:][12], e.g. use git's command line option `-s` to append it automatically to your commit message:
     `git commit -s -m 'This is my good	commit message'`
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
* [HScope for Android](https://www.martinloren.com/hscope/) A one-channel basic version is available free of charge (with in-app purchases).
