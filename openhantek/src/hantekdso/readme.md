# Content
This directory contains the heart of OpenHantek, the `HantekDSOControl` class
and all model definitions.

## HantekDSOControl
The `HantekDSOControl` class manages all device settings (gain, offsets, channels, etc)
and outputs `DSOSamples` via `getLastSamples()`. Observers are notified of a new set of
available samples via the signal `samplesAvailable()`.
Current device settings are stored in the `controlsettings` field and retriveable with the
corresponding getter `getDeviceSettings()`.

`HantekDSOControl` may only contain state fields to realize the fetch samples / modify settings loop.

## Model
A model needs a `ControlSpecification`, which
describes what specific Hantek protocol commands are to be used. All known
models are specified in the subdirectory `models`.

# Namespace
Relevant classes in here are in the `DSO` namespace.

# Dependency
* Files in this directory depend on structs in the `usb` folder.
* Files in this directory depend on structs in the `hantekprotocol` folder.
