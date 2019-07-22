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

The heart of OpenHantek is the `src/hantekdso` folder and its `hantekdsocontrol` class. All supported models, 
based on the `DsoModel` class, are implemented within a subfolder `src/hantekdso/models` and automatically register
themself to a `ModelRegistry` class. A model is based on (contains) a specification, the `ControlSpecification` class.

The `hantekdsocontrol` class keeps track of the devices current state (samplerate, selected gain, activated channels,
etc) via the `ControlSettings` class and field.
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
`GlScope` works normally for **OpenGL 3.2+** and OpenGL ES 2.0+ but if it detects **OpenGL 2.1+** and OpenGL ES 1.2+ on older platforms it switches to a legacy implementation. If both OpenGL and OpenGL Es are present, OpenGL will be prefered, but can be overwritten by the user via a command flag.

### Export

All export related funtionality is within *src/exporting*.

The following exporters are implemented:

* Export to comma separated value file (CSV): Write to a user selected file,
* Export to an image/pdf: Writes an image/pdf to a user selected file,
* Print exporter: Creates a printable document and opens the print dialog.

All export classes (exportcsv, exportimage, exportprint) implement the
ExporterInterface and are registered to the ExporterRegistry in the main.cpp.

Some export classes are still using the legacyExportDrawer class to
draw the grid and paint all the labels, values and graphs.

The plan is to retire this legacy class and replace the paint code with
a `GlScope` class shared OpenGL drawing code for at least the grid and the
scope graphs.

## Data flow

* Raw 8-bit ADC values are collected in `HantekDsoControl::run()` and converted in `HantekDsoControl::convertRawDataToSamples()` to real-world double samples (scaled with voltage and sample rate). The 10X oversampling for slower sample rates is done here. Also overdriving of the inputs is detected. The conversion uses either the factory calibration values from EEPROM or from a user supplied config file. Read more about [calibration](https://github.com/Ho-Ro/Hantek6022API/blob/master/README.md#create-calibration-values-for-openhantek).
  * `swTrigger()`
    * which checks if the signal is triggered and calculates the starting point for a stable display.
  * `triggering()` handles the trigger mode:
    * If the **trigger condition is false** and the **trigger mode is Normal** then we reuse the last triggered samples so that voltage and spectrum traces as well as the measurement at the scope's bottom lines are frozen until the trigger condition becomes true again.
    * If the **trigger condition is false** and the **trigger mode is not Normal** then we display a free running trace and discard the last saved samples.
* The converted samples are emitted to PostProcessing::input() via signal/slot.
* PostProzessing calls all processors that were registered in `main.cpp`.
  * `MathchannelGenerator::process()`
    * which creates a third MATH channel as one of these data sample combinations: 
      `CH1 + CH2`, `CH1 - CH2`, `CH2 - CH1`, `CH1 * CH2`, `CH1 AC` or `CH2 AC`.
  * `SpectrumGenerator::process()`
    * For each active channel:
      * Calculate the DC (average), AC (rms) and effective value ( sqrt( DC² + AC² ) ).
      * Apply a user selected window function and scale the result accordingly.
      * Calculate the autocorrelation to get the frequency of the signal. This is quite inaccurate at high frequencies. In these cases the first peak value of the spectrum is used.
      * Calculate the spectrum of the AC part of the signal scaled as dBV.
  * `GraphGenerator::process()`
    * which works either in TY mode and creates two types of traces:
      * voltage over time `GraphGenerator::generateGraphsTYvoltage()`
      * spectrum over frequency `GraphGenerator::generateGraphsTYspectrum()`
    * or in XY mode and creates a voltage over voltage trace `GraphGenerator::generateGraphsXY()`.
    * `GraphGenerator::generateGraphsTYvoltage()` creates up to three (CH1, CH2, MATH) voltage traces.
    * `GraphGenerator::generateGraphsTYspectrum()` creates up to three (CH1, CH2, MATH) spectral traces.

t.b.c.
