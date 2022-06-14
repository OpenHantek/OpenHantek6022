// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "devicelistentry.h"
#include <QAbstractTableModel>

class FindDevices;

/**
 * Provides a Model for the Qt Model/View concept. The {@see FindDevices} is required
 * to update the list of available devices.
 */
class DevicesListModel : public QAbstractTableModel {
  public:
    explicit DevicesListModel( FindDevices *findDevices, int verboseLevel = 0 );
    // QAbstractItemModel interface
    int rowCount( const QModelIndex &parent ) const override;
    int columnCount( const QModelIndex &parent ) const override;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;
    QVariant data( const QModelIndex &index, int role ) const override;
    void updateDeviceList();

  private:
    std::vector< DeviceListEntry > entries;
    FindDevices *findDevices;
    int verboseLevel = 0;
};
