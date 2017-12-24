#pragma once

#include <QAbstractTableModel>
#include "devicelistentry.h"

class FindDevices;

/**
 * Provides a Model for the Qt Model/View concept. The {@see FindDevices} is required
 * to update the list of available devices.
 */
class DevicesListModel: public QAbstractTableModel {
public:
    DevicesListModel(FindDevices* findDevices);
    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    void updateDeviceList();
private:
    std::vector<DeviceListEntry> entries;
    FindDevices* findDevices;
};
