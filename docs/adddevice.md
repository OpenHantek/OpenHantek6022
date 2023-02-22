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
The `utils/udev_rules/60-openhantek.rules` file needs the usb vendor/device id to add access permissions.

## The hantek protocol
The hantek protocol itself is encoded in the `src/hantekprotocol` files.
If your device needs other or slightly altered packets, you would need to modify those files.

## Add your model information
You will only need to touch files within `openhantek/src/hantekdso/models`.

1. Create a new class with your model name and inherit from `DSOModel`:

``` c++
struct ModelDSO2090 : public DSOModel {
    static const int ID = 0x2090; // Freely chooseable but unique id
    ModelDSO2090();
    void applyRequirements(HantekDsoControl* dsoControl) const override;
};
```

2. Implement the constructor of your class, where you need to supply the constructor of `DSOModel` with
   some information. The `DSOModel` constructor looks like this:

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

3. Add your device specific constants via the `specification` field, for instance:

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

4. The actual commands that are send, need to be defined as well, for instance:

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

5. Add an instance of your class to the cpp file. The `DSOModel` constructor will register
   your new model automatically to the ModelRegistry:

```
static ModelDSO2090 modelInstance;
```

## Example: New Device Model DSO-6021
As an example for adding a new device you can check commit 77ba4ad and read about
[adding the new model DSO-6021](https://github.com/Ho-Ro/Hantek6022API/blob/main/docs/DIY_6021/DIY_6021.md)
at my [firmware project](https://github.com/Ho-Ro/Hantek6022API).
