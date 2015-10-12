# OpenHantek [![Build Status](https://travis-ci.org/OpenHantek/openhantek.svg)](https://travis-ci.org/OpenHantek/openhantek)
OpenHantek is a free software for Hantek (Voltcraft/Darkwire/Protek/Acetech) USB digital storage oscilloscopes based on HantekDSO and has started as an alternative to the official Hantek DSO software for Linux users.

<img alt="Image of main window" src="doc/screenshot_mainwindow.png">

Tested models so far:
* DSO-2090

Supported operating systems:
* Linux
* MacOSX (The lack of udev requires you to load the firmware by hand for now)

## Building OpenHantek from source
You need the following packages, to build OpenHantek from source:
* CMake 2.8.12+
* Qt 5+
* FFTW 3+
* libusb 1.x

For debian based systems (Ubuntu, Mint) install named requirements like this:
> apt-get install cmake libqt5-dev libfftw3-dev

For rpm based distributions (Fedora) use this command:
> dnf install cmake qt5-qtbase-gui qt5-qttools-devel qt5-qttranslations fftw-devel libusbx-devel

After you've installed the requirements run the following commands inside the directory of this package:
> qmake <br>
> make <br>
> make install

You can specify a prefix when running qmake:
> qmake PREFIX=/usr

## Firmware
Your DSO does not store its firmware permanently and have to be send to the device each time it is connected. Because of copyright reasons we cannot ship the firmware with this software. You have to extract the firmware using openhantek-extractfw and add some rules to udev.

### Getting the Windows drivers
Before using OpenHantek you have to extract the firmware from the official Windows drivers. You can get them from the <a href="http://www.hantek.ru/download.html">Hantek website</a> or automatically download them with the script _fwget.sh_.

### The firmware extraction tool
Install libbfd (Ubuntu) / binutils (Fedora) and build the tool by typing:
> make

After building it, you can just run the fwget.sh script inside the openhantek-extractfw directory:
> sudo ./fwget.sh /usr/local/share/hantek/

You can also do it manually by placing the DSO*1.SYS file into the same directory and running the built binary:
> ./openhantek-extractfw &lt;driver file&gt;

This should create two .hex files that should be placed into /usr/local/share/hantek/.

### Installing the firmware
* Copy the 90-hantek.rules file to /etc/udev/rules.d/.
* install fxload (fxload is a program which downloads firmware to USB  devices  based  on
       AnchorChips  EZ-USB, Cypress EZ-USB FX, or Cypress EZ-USB FX2 microcontrollers.)
* Add your current user to the **plugdev** group.

## Contribute
Please use Github Issues to report any problems or enhancements or send us pull requests. Some random notes:
   - Read [how to properly contribute to open source projects on GitHub][10].
   - Use a topic branch to easily amend a pull request later, if necessary.
   - Write [good commit messages][11].
   - Squash commits on the topic branch before opening a pull request.
   - Use the same coding style and spacing.
   - Open a [pull request][12] that relates to but one subject with a clear title and description
     
[10]: http://gun.io/blog/how-to-github-fork-branch-and-pull-request
[11]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html
[12]: https://help.github.com/articles/using-pull-requests
