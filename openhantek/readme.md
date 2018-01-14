# OpenHantek developer documentation

This document explains the basic data flow and concepts used and should be amended
whenever a new concept is added to the code base.
Detailed information about an implementation are to be found in the specific directory readmes though.

## Directory structure

The *res* folder contains mostly binary resource data that are bundled with the executable,
like icons, images and firmwares.

The *translations* folder contains translations in the Qt format. Use Qt linguist
as a handy graphical interface to translate OpenHantek.

The source code within *src* can be divided into a **core**, that is responsible for device communication,
data acquisition and post processing and the **graphical interface** with several custom widgets,
a configuration interface and an OpenGL renderer.

### Core structure

The raw device communcation takes place in the *src/usb* directory, especially via the `USBDevice` class.
To find suitable devices, the `FindDevices` class in the same folder is used. Firmware upload is realized
via the `ezusb` helper methods and the `UploadFirmware` class.

The hantek protocol structures and constants are defined within `src/hantekprotocol`.

The heart of OpenHantek is the `src/hantekdso` folder and its `hantekdsocontrol` class. All supported
models, based on the `DsoModel` class, are implemented within a subfolder `src/hantekdso/models` and automatically register themself
to a `ModelRegistry` class. A model is based on (contains) a specification, the `ControlSpecification` class.

The `hantekdsocontrol` class keeps track of the devices current state (samplerate, selected gain, activated channels, etc)
via the `ControlSettings` class and field.
It outputs the channel separated unprocessed samples via a `samplesAvailable(DSOsamples*)` signal.

Before the data is presented to the GUI it arrives in the `src/post/postprocessing` class. Several post
processing classes are to be found in this directory as well.

### Graphical interface structure

The initial dialog for device selection is realized in *src/selectdevice* where several models
and dialogs are implemented. This is basically a graphical wrapper around the `src/usb/finddevices` class.

You will find the configuration dialog pages to be implemented in *src/configdialog*.

Custom widgets like a LevelSlider with a unit suffix reside in *src/widgets*, the custom main window docks are
in *src/docks*.

The code that is responsible for exporting data to images or to the printer is stored in *src/exporting*.

The main window itself doesn't do and shouldn't do much more than connecting signals/slots between the core part
and the graphical part.

All OpenGL rendering takes place in the `GlScope` class. A helper class `GlScopeGraph` contains exactly one
data sample snapshot including all channels for voltage and spectrum and a pointer to the respective GPU buffer.
`GlScope` works for OpenGL 3.2 and OpenGL ES 2.0, but this needs to be decided at compile time. Usually Qt
selects the right interface.

## Data flow

To be written
