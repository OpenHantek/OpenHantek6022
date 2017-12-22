---
layout: default
---
# How to add a new device
We only accept new devices whoms firmware is hantek protocol compatible.
Codewise you will only need to touch files within `openhantek/src/hantek`.

## Firmware and usb access
The firmware goes to `openhantek/res/firmware` in the hex format. Please keep to the filename
convention devicename-firmware.hex and devicename-loader.hex.
The `openhantek/res/firmwares.qrc` should list the new files.
The firmware/60-hantek.rules file needs the usb vendor/device id to add access permissions.

## The hantek protocol
The hantek protocol itself is encoded in the `bulkStructs.h` and `controlStructs.h` files.
If your device needs other or slighly altered packets, you would need to modify those files.

## Add your model information
The `models.h` file needs the model information. Add your model to the ``enum Model`` and
also to the ``std::list<DSOModel> supportedModels`` list. A list item is a ``DSOModel``
with the following constructor:

``` c++
DSOModel(Model model, long vendorID, long productID, long vendorIDnoFirmware, long productIDnoFirmware,
             std::string firmwareToken, const std::string name)
```

You need to find out the usb vendor id and product id for your digital oscilloscope after it has received
the firmware (for ``long vendorID``, ``long productID``) and before it has a valid firmware
(for ``long vendorIDnoFirmware``, ``long productIDnoFirmware``).

The firmware token is just the devicename part of the firmware
(remember that we used `devicename-firmware.hex` and `devicename-loader.hex`).

The last parameter is the user visible name of the device.

## Add your device specifications and capabilities

This is not ready yet for easy device addition. At the moment you need to add your device specifications
in the `hantekdsocontrol.cpp` constructor.

