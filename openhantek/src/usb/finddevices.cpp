// SPDX-License-Identifier: GPL-2.0+

#include "finddevices.h"

#include <QCoreApplication>
#include <QDebug>
#include <QList>
#include <QTemporaryFile>

#include <algorithm>
#include "ezusb.h"
#include "utils/printutils.h"
#ifdef __FreeBSD__
	#include <libusb.h>
#else
	#include <libusb-1.0/libusb.h>
#endif

#include "modelregistry.h"

FindDevices::FindDevices(libusb_context *context) : context(context) {}

// Iterate through all usb devices
int FindDevices::updateDeviceList() {
    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(context, &deviceList);
    if (deviceCount < 0) {
        return (int) deviceCount;
    }

    ++findIteration;
    int changes = 0;

    for (ssize_t deviceIterator = 0; deviceIterator < deviceCount; ++deviceIterator) {
        libusb_device *device = deviceList[deviceIterator];
        // Get device descriptor
        struct libusb_device_descriptor descriptor;
        libusb_get_device_descriptor(device, &descriptor);
        DeviceList::const_iterator inList = devices.find(USBDevice::computeUSBdeviceID(device));

        if ( inList != devices.end()) {
            inList->second->setFindIteration(findIteration);
            //continue;
        }

        for (DSOModel* model : ModelRegistry::get()->models()) {
            // Check VID and PID for firmware flashed devices
            bool supported = descriptor.idVendor == model->vendorID && descriptor.idProduct == model->productID;
            // Devices without firmware have different VID/PIDs
            supported |= descriptor.idVendor == model->vendorIDnoFirmware && descriptor.idProduct == model->productIDnoFirmware;
            if (supported) {
                ++changes;
                devices[USBDevice::computeUSBdeviceID(device)] = std::unique_ptr<USBDevice>(new USBDevice(model, device, findIteration));
            }
        }
    }

    // Remove non existing devices
    for (DeviceList::iterator it=devices.begin();it!=devices.end();) {
        if (it->second->getFindIteration() != findIteration) {
            ++changes;
            it = devices.erase(it);
        } else {
            ++it;
        }
    }

// TODO: test on FreeBSD
#if 1
    libusb_free_device_list( deviceList, false );
#else
    #if !defined(__FreeBSD__)
        /*
        ToDo: This introduces a potential resource leak if not executed
        on FreeBSD. It seems there is a reference counting problem when
        using libusb on FreeBSD.
        */
        libusb_free_device_list( deviceList, false );
    #endif
#endif

    return changes;
}

const FindDevices::DeviceList* FindDevices::getDevices()
{
    return &devices;
}

std::unique_ptr<USBDevice> FindDevices::takeDevice(UniqueUSBid id)
{
    DeviceList::iterator i = devices.find(id);
    if (i==devices.end()) return nullptr;
    return std::move(i->second);
}
