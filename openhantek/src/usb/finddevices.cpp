// SPDX-License-Identifier: GPL-2.0+

#include "finddevices.h"

#include <QCoreApplication>
#include <QDebug>
#include <QList>
#include <QTemporaryFile>

#include "ezusb.h"
#include "utils/printutils.h"
#include <libusb-1.0/libusb.h>

#include "models.h"

FindDevices::FindDevices(libusb_context *context) : context(context) {}

// Iterate through all usb devices
std::list<std::unique_ptr<USBDevice>> FindDevices::findDevices() {
    std::list<std::unique_ptr<USBDevice>> devices;

    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(context, &deviceList);
    if (deviceCount < 0) {
        errorMessage = QCoreApplication::translate("", "Failed to get device list");
        return devices;
    }

    noAccessDevices = false;
    int noAccessDeviceCount = 0;

    for (ssize_t deviceIterator = 0; deviceIterator < deviceCount; ++deviceIterator) {
        libusb_device *device = deviceList[deviceIterator];
        // Get device descriptor
        struct libusb_device_descriptor descriptor;
        if (libusb_get_device_descriptor(device, &descriptor) < 0) continue;

        for (DSOModel* model : supportedModels) {
            // Check VID and PID for firmware flashed devices
            if (descriptor.idVendor == model->vendorID && descriptor.idProduct == model->productID) {
                devices.push_back(std::unique_ptr<USBDevice>(new USBDevice(model, device)));
                break;
            }
            // Devices without firmware have different VID/PIDs
            if (descriptor.idVendor == model->vendorIDnoFirmware && descriptor.idProduct == model->productIDnoFirmware) {
                devices.push_back(std::unique_ptr<USBDevice>(new USBDevice(model, device)));
                break;
            }
        }
    }

    if (noAccessDeviceCount == deviceCount) {
        noAccessDevices = true;
        errorMessage =
            QCoreApplication::translate("", "Please make sure to have read/write access to your usb device. On "
                                            "linux you need to install the correct udev file for example.");
    }

    libusb_free_device_list(deviceList, true);
    return devices;
}

const QString &FindDevices::getErrorMessage() const { return errorMessage; }

bool FindDevices::allDevicesNoAccessError() const { return noAccessDevices; }
