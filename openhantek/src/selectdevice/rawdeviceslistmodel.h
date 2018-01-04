// SPDX-License-Identifier: GPL-2.0+
#pragma once

#include <QAbstractTableModel>
#include "rawdevicelistentry.h"
#include "usb/usbdevice.h"

/**
 * Provides a Model for the Qt Model/View concept. The {@see FindDevices} is required
 * to update the list of available devices.
 */
class RawDevicesListModel: public QAbstractTableModel {
public:
    RawDevicesListModel(libusb_context *context, QObject *parent = 0);
    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    void updateDeviceList();

    enum Roles {
        ProductIDRole = Qt::UserRole+0,
        VendorIDRole = Qt::UserRole+1,
        AccessRole = Qt::UserRole+2,
        DeviceNameRole =  Qt::UserRole+3,
        EntryPointerRole = Qt::UserRole+4
    };
private:
    std::vector<RawDeviceListEntry> entries;
    libusb_context *context;
};
