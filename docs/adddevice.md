---
layout: default
---
# How to add a new device
We only accept new devices whoms firmware is hantek protocol compatible.
Codewise you will only need to touch files within `openhantek/src/hantekdso`.

## Firmware and usb access
The firmware goes to `openhantek/res/firmware` in the hex format. Please keep to the filename
convention devicename-firmware.hex and devicename-loader.hex.
The `openhantek/res/firmwares.qrc` should list the new files.
The firmware/60-hantek.rules file needs the usb vendor/device id to add access permissions.

## The hantek protocol
The hantek protocol itself is encoded in the `src/hantekprotocol` files.
If your device needs other or slighly altered packets, you would need to modify those files.

## Add your model information
You will only need to touch files within `openhantek/src/hantekdso/models`.

Create a new class with your model name and inherit from `DSOModel`. Add an instance of your new class
to the `supportedModels` list in `models.cpp`.

The following code shows the constructor of `DSOModel` that needs to be supplied with model specific data:

``` c++
DSOModel(int ID, long vendorID, long productID, long vendorIDnoFirmware, long productIDnoFirmware,
             std::string firmwareToken, const std::string name)
```

* You need to find out the usb vendor id and product id for your digital oscilloscope after it has received
  the firmware (for ``long vendorID``, ``long productID``) and before it has a valid firmware
  (for ``long vendorIDnoFirmware``, ``long productIDnoFirmware``).
* The firmware token is just the devicename part of the firmware
  (remember that we used `devicename-firmware.hex` and `devicename-loader.hex`).
* The last parameter is the user visible name of the device.

Add your device specific constants via the `specification` field, for instance:

``` c++
    specification.samplerate.single.base = 50e6;
    specification.samplerate.single.max = 50e6;
    specification.samplerate.single.maxDownsampler = 131072;
    specification.samplerate.single.recordLengths = {UINT_MAX, 10240, 32768};
    specification.samplerate.multi.base = 100e6;
    specification.samplerate.multi.max = 100e6;
    specification.samplerate.multi.maxDownsampler = 131072;
    specification.samplerate.multi.recordLengths = {UINT_MAX, 20480, 65536};
```

The actual commands that are send, need to be defined as well, for instance:

``` c++
    specification.command.control.setOffset = CONTROL_SETOFFSET;
    specification.command.control.setRelays = CONTROL_SETRELAYS;
    specification.command.bulk.setGain = BulkCode::SETGAIN;
    specification.command.bulk.setRecordLength = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setChannels = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setSamplerate = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setTrigger = BulkCode::SETTRIGGERANDSAMPLERATE;
    specification.command.bulk.setPretrigger = BulkCode::SETTRIGGERANDSAMPLERATE;
```


