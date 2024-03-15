### [Linux](#linux)
For Debian (stretch and newer), Ubuntu 17.04+ and Mint 17+ and other deb based distributions install named requirements like this:
> apt install g++ make cmake fakeroot qttools5-dev libfftw3-dev binutils-dev libusb-1.0-0-dev libqt5opengl5-dev mesa-common-dev libgl1-mesa-dev libgles2-mesa-dev

For distributions using dnf package manager (Fedora 21+) use this command:
> dnf install make cmake fakeroot gcc-c++ qt5-qtbase-gui qt5-qttools-devel qt5-qttranslations fftw-devel binutils-devel libusb-devel mesa-libGL-devel mesa-libGLES-devel

For OpenSUSE and related distributions use this command
> zypper install make cmake fakeroot gcc-c++ libqt5-qtbase libqt5-qttools libqt5-qttranslations libusb-1_0 Mesa-libGL1 Mesa-libGLESv2 fftw3 

The script [`LinuxSetup_AsRoot`](../LinuxSetup_AsRoot) installs all build requirements automatically.

After you've installed the requirements run the following commands inside the directory of this package:

    mkdir build
    cd build
    cmake ..
    make -j4

or execute the script [`LinuxBuild`](../LinuxBuild) that configures the build, builds the binary and finally creates the packages (deb, rpm and tgz) that can be installed as described in the next paragraphs.
If you make small changes to the local source code, it is sufficient to call `make -j4` or `make -j4 package` in the `build` directory.

After success you can test the newly built program `openhantek/OpenHantek`.
Due to the included debug information this file is quite big (~20 MB), but the size can be reduced with `strip openhantek/OpenHantek` if you want to put it into a user directory. 
If you do not install the program, you need to copy the file `utils/udev_rules/60-openhantek.rules` to `/etc/udev/rules.d/` yourself,
and replug your device, otherwise you will not have the correct USB permissions to access the device.

You can install the program with `sudo make install`, but it is highly recommended to create a debian package,
which allows a clear installation and removal of the package:

    rm -f packages/*
    fakeroot make -j4 package
    sudo apt install packages/openhantek_*_amd64.deb

If you detect that icons are not displayed correctly, please check if the Qt SVG library is installed on your system.
The Linux systems mentioned above include this lib when you install according to the provided lists.
However, an [alpine linux](https://alpinelinux.org/) user [reported](https://github.com/OpenHantek/OpenHantek6022/issues/42#issuecomment-564329632) that he had to install `qt5-qtsvg` separately.

#### CI Build on GitHub Actions

The local build and test is done on an up-to-date Debian stable; on every push a building process is run externally by [GitHub Actions](https://github.com/OpenHantek/OpenHantek6022/actions)
who provides [these Ubuntu 2004 environments](https://github.com/actions/runner-images/blob/main/images/linux/Ubuntu2004-Readme.md).
Please check also the file [build_check.yml](https://github.com/OpenHantek/OpenHantek6022/blob/main/.github/workflows/build_check.yml) for info about the building process.

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

### [macOS](#macos)
Building should work on a recent macOS 11 version.

We recommend homebrew to install the required libraries.

    git submodule update --init --recursive
    brew update
    brew install libusb fftw qt5 cmake binutils create-dmg

If you want to build an OSX bundle make sure the option in `openhantek/CMakeLists.txt` is set accordingly:

    option(BUILD_MACOSX_BUNDLE "Build MacOS app bundle" ON)

After you've installed the requirements run the following commands inside the top directory of this package:

    mkdir -p build
    rm -rf build/*
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
    python ../../utils/macdeployqtfix/macdeployqtfix.py OpenHantek.app/Contents/MacOS/OpenHantek $(brew --prefix qt5)
    #
    # finally create OpenHantek.dmg from OpenHantek.app
    create-dmg --volname OpenHantek --volicon ../../openhantek/res/images/OpenHantek.icns --window-pos 200 120 \
      --window-size 800 400 --icon-size 100 --icon "OpenHantek.app" 200 190 --skip-jenkins \
      --hide-extension "OpenHantek.app" --app-drop-link 600 185 --eula ../../LICENSE OpenHantek.dmg OpenHantek.app
    #

This code proposal is based on [the info from @warpme](https://github.com/OpenHantek/OpenHantek6022/issues/314#issuecomment-1200268722)
about building on macOS 11.6.8 + Xcode 12.4 (12D4e).

#### CI Build on GitHub Actions

As I do not use macOS for development the building is done externally by [GitHub Actions](https://github.com/OpenHantek/OpenHantek6022/actions)
who provides [these macOS 11 environments](https://github.com/actions/runner-images/blob/main/images/macos/macos-11-Readme.md).
Please check also the file [build_check.yml](https://github.com/OpenHantek/OpenHantek6022/blob/main/.github/workflows/build_check.yml) for info about the building process.

----

### [Windows](#windows)

We highly recommend to use QtCreator to build this software. All reported issues regarding other IDEs
will be closed as invalid!

* Open the project in QtCreator
* Compile the software

Hints for Visual Studio 2015/2017/2019 users:

* Install the right Qt package that matches your Visual Studio installation.
* Build for 64bit. 32bit builds theoretically work, but you are on your own then.
* Use the **CMake GUI** to setup all required Qt include and library paths.

#### CI Build on GitHub Actions

As I do not use Windows for development the building is done externally by [GitHub Actions](https://github.com/OpenHantek/OpenHantek6022/actions)
who provides [these Windows environments](https://github.com/actions/virtual-environments/blob/main/images/win/Windows2019-Readme.md).
Please check also the file [build_check.yml](https://github.com/OpenHantek/OpenHantek6022/blob/main/.github/workflows/build_check.yml) for info about the building process with either MINGW or MSVC.
Starting with the update to Visual Studio 2019 only 64bit builds are provided.

#### Signed WinUSB driver for Hantek 6022BE/BL

- The signed `.inf` file `OpenHantek.inf` for all devices - [provided by VictorEEV](https://www.eevblog.com/forum/testgear/hantek-6022be-20mhz-usb-dso/msg4418107/#msg4418107)
and [updated](https://github.com/OpenHantek/OpenHantek6022/pull/323) by [gitguest0](https://github.com/gitguest0) - 
is available in the `openhantek_xxx_win_x64.zip` [binary distribution](https://github.com/OpenHantek/OpenHantek6022/releases) in directory `driver`.

- Right-click on `OpenHantek.inf` and select "install" from the pull-down menu.

- The Device Manager will show (under "Universal Serial Bus devices") the name and state according to the firmware loaded (e.g. `Hantek 6022BE - Loader`, `Hantek 6022BE - OpenHantek`).
The [PulseView/sigrok-cli](https://sigrok.org/) firmware is also recognized (e.g. `Hantek 6022BE - Sigrok`).

#### Microsoft Windows USB driver install (with Zadig)

It is highly recommended to use the `.inf` file, but it is also possible to alternatively use the [**Zadig**](docs/build.md#microsoft-windows-usb-driver-install-with-zadig) tool
and follow the good [step-by-step tutorial](docs/OpenHantek6022_zadig_Win10.pdf) provided by [DaPa](https://github.com/DaPa).

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

