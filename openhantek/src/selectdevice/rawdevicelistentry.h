// SPDX-License-Identifier: GPL-2.0+
#pragma once

#include <QString>
#include "usb/usbdevice.h"
#include "dsomodel.h"
/**
 * Represents an entry in the {@link DevicesListModel}.
 */
struct RawDeviceListEntry {
    long productId;
    long vendorId;
    bool access;
    DSOModel* baseModel=nullptr;
    QString devicename;
    QString deviceinfo;
};
