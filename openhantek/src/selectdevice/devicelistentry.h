#pragma once

#include <QString>
#include "usb/usbdevice.h"

/**
 * Represents an entry in the {@link DevicesListModel}.
 */
struct DeviceListEntry {
    UniqueUSBid id;
    QString name;
    bool canConnect;
    bool needFirmware;
    QString errorMessage;
    QString getStatus() const {
        return errorMessage.size()? errorMessage : (canConnect?"Ready":(needFirmware?"Firmware upload":"Cannot connect"));
    }
};
