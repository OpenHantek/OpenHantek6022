// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <libusb-1.0/libusb.h>
#include <memory>
#include <QString>

#include "definitions.h"
#include "usbdevice.h"

/**
 * @brief Search for Hantek devices and connect to the selected one.
 *
 * At the moment this class connects to the first found devic automatically.
 */
class FindDevices {
public:
    FindDevices(libusb_context *context=nullptr);
    std::list<std::unique_ptr<USBDevice>> findDevices();
    const QString& getErrorMessage() const;
    bool allDevicesNoAccessError() const;
private:
    libusb_context *context;      ///< The usb context used for this device
    QString errorMessage;
    bool noAccessDevices=false;
};
