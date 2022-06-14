// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "usb/scopedevice.h"
#include <QString>

/**
 * Represents an entry in the {@link DevicesListModel}.
 */
struct DeviceListEntry {
    UniqueUSBid id;
    QString name;
    bool canConnect = false;
    bool needFirmware = false;
    QString errorMessage;
    QString getStatus() const {
        return errorMessage.size() ? errorMessage
                                   : ( canConnect ? "Ready" : ( needFirmware ? "Firmware upload" : "Cannot connect" ) );
    }
};
