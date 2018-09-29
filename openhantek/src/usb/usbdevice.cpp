// SPDX-License-Identifier: GPL-2.0+

#include <QCoreApplication>
#include <QList>
#include <iostream>

#include "usbdevice.h"

#include "hantekdso/dsomodel.h"
#include "hantekprotocol/bulkStructs.h"
#include "hantekprotocol/controlStructs.h"

#include <QCoreApplication>

QString libUsbErrorString(int error) {
    switch (error) {
    case LIBUSB_SUCCESS:
        return QCoreApplication::tr("Success (no error)");
    case LIBUSB_ERROR_IO:
        return QCoreApplication::tr("Input/output error");
    case LIBUSB_ERROR_INVALID_PARAM:
        return QCoreApplication::tr("Invalid parameter");
    case LIBUSB_ERROR_ACCESS:
        return QCoreApplication::tr("Access denied (insufficient permissions)");
    case LIBUSB_ERROR_NO_DEVICE:
        return QCoreApplication::tr("No such device (it may have been disconnected)");
    case LIBUSB_ERROR_NOT_FOUND:
        return QCoreApplication::tr("Entity not found");
    case LIBUSB_ERROR_BUSY:
        return QCoreApplication::tr("Resource busy");
    case LIBUSB_ERROR_TIMEOUT:
        return QCoreApplication::tr("Operation timed out");
    case LIBUSB_ERROR_OVERFLOW:
        return QCoreApplication::tr("Overflow");
    case LIBUSB_ERROR_PIPE:
        return QCoreApplication::tr("Pipe error");
    case LIBUSB_ERROR_INTERRUPTED:
        return QCoreApplication::tr("System call interrupted (perhaps due to signal)");
    case LIBUSB_ERROR_NO_MEM:
        return QCoreApplication::tr("Insufficient memory");
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return QCoreApplication::tr("Operation not supported or unimplemented on this platform");
    default:
        return QCoreApplication::tr("Other error");
    }
}

UniqueUSBid USBDevice::computeUSBdeviceID(libusb_device *device) {
    UniqueUSBid v = 0;
    libusb_get_port_numbers(device, (uint8_t *)&v, sizeof(v));
    return v;
}

USBDevice::USBDevice(DSOModel *model, libusb_device *device, unsigned findIteration)
    : model(model), device(device), findIteration(findIteration), uniqueUSBdeviceID(computeUSBdeviceID(device)) {
    libusb_ref_device(device);
    libusb_get_device_descriptor(device, &descriptor);
}

bool USBDevice::connectDevice(QString &errorMessage) {
    if (needsFirmware()) return false;
    if (isConnected()) return true;

    // Open device
    int errorCode = libusb_open(device, &(handle));
    if (errorCode != LIBUSB_SUCCESS) {
        handle = nullptr;
        errorMessage = QCoreApplication::translate("", "Couldn't open device: %1").arg(libUsbErrorString(errorCode));
        return false;
    }

    // Find and claim interface
    errorCode = LIBUSB_ERROR_NOT_FOUND;
    libusb_config_descriptor *configDescriptor;
    libusb_get_config_descriptor(device, 0, &configDescriptor);
    for (int interfaceIndex = 0; interfaceIndex < (int)configDescriptor->bNumInterfaces; ++interfaceIndex) {
        const libusb_interface *interface = &configDescriptor->interface[interfaceIndex];
        if (interface->num_altsetting < 1) continue;

        const libusb_interface_descriptor *interfaceDescriptor = &interface->altsetting[0];
        if (interfaceDescriptor->bInterfaceClass == LIBUSB_CLASS_VENDOR_SPEC &&
            interfaceDescriptor->bInterfaceSubClass == 0 && interfaceDescriptor->bInterfaceProtocol == 0 &&
            interfaceDescriptor->bNumEndpoints == 2) {
            errorCode = claimInterface(interfaceDescriptor, HANTEK_EP_OUT, HANTEK_EP_IN);
            break;
        }
    }

    libusb_free_config_descriptor(configDescriptor);

    if (errorCode != LIBUSB_SUCCESS) {
        errorMessage = QString("%1 (%2:%3)")
                           .arg(libUsbErrorString(errorCode))
                           .arg(libusb_get_bus_number(device), 3, 10, QLatin1Char('0'))
                           .arg(libusb_get_device_address(device), 3, 10, QLatin1Char('0'));
        return false;
    }

    return true;
}

USBDevice::~USBDevice() {
	disconnectFromDevice();
#if defined(_WIN32) || defined(_WIN64)
	if (device != nullptr) libusb_unref_device(device);
	device = nullptr;
#endif
}

int USBDevice::claimInterface(const libusb_interface_descriptor *interfaceDescriptor, int endpointOut, int endPointIn) {
    int errorCode = libusb_claim_interface(this->handle, interfaceDescriptor->bInterfaceNumber);
    if (errorCode < 0) { return errorCode; }

    interface = interfaceDescriptor->bInterfaceNumber;

    // Check the maximum endpoint packet size
    const libusb_endpoint_descriptor *endpointDescriptor;
    this->outPacketLength = 0;
    this->inPacketLength = 0;
    for (int endpoint = 0; endpoint < interfaceDescriptor->bNumEndpoints; ++endpoint) {
        endpointDescriptor = &(interfaceDescriptor->endpoint[endpoint]);
        if (endpointDescriptor->bEndpointAddress == endpointOut) {
            this->outPacketLength = endpointDescriptor->wMaxPacketSize;
        } else if (endpointDescriptor->bEndpointAddress == endPointIn) {
            this->inPacketLength = endpointDescriptor->wMaxPacketSize;
        }
    }
    return LIBUSB_SUCCESS;
}

void USBDevice::disconnectFromDevice() {
    if (!device) return;

    if (this->handle) {
        // Release claimed interface
        if (this->interface != -1) libusb_release_interface(this->handle, this->interface);
        this->interface = -1;

        // Close device handle
        libusb_close(this->handle);
    }
    this->handle = nullptr;

#if !defined(_WIN32) || !defined(_WIN64)
    libusb_unref_device(device);
#endif

    emit deviceDisconnected();
}

bool USBDevice::isConnected() { return this->handle != 0; }

bool USBDevice::needsFirmware() {
    return this->descriptor.idProduct != model->productID || this->descriptor.idVendor != model->vendorID;
}

int USBDevice::bulkTransfer(unsigned char endpoint, const unsigned char *data, unsigned int length, int attempts,
                            unsigned int timeout) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = LIBUSB_ERROR_TIMEOUT;
    int transferred = 0;
    for (int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
        errorCode =
            libusb_bulk_transfer(this->handle, endpoint, (unsigned char *)data, (int)length, &transferred, timeout);

    if (errorCode == LIBUSB_ERROR_NO_DEVICE) disconnectFromDevice();
    if (errorCode < 0)
        return errorCode;
    else
        return transferred;
}

int USBDevice::bulkReadMulti(unsigned char *data, unsigned length, int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = this->inPacketLength;
    unsigned int packet, received = 0;
    for (packet = 0; received < length && errorCode == this->inPacketLength; ++packet) {
        errorCode = this->bulkTransfer(HANTEK_EP_IN, data + packet * this->inPacketLength,
                                       qMin(length - received, (unsigned int)this->inPacketLength), attempts,
                                       HANTEK_TIMEOUT_MULTI);
        if (errorCode > 0) received += (unsigned)errorCode;
    }

    if (received > 0)
        return (int)received;
    else
        return errorCode;
}

int USBDevice::controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length,
                               int value, int index, int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = LIBUSB_ERROR_TIMEOUT;
    for (int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
        errorCode = libusb_control_transfer(this->handle, type, request, value, index, data, length, HANTEK_TIMEOUT);

    if (errorCode == LIBUSB_ERROR_NO_DEVICE) disconnectFromDevice();
    return errorCode;
}




