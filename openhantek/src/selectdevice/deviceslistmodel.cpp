// SPDX-License-Identifier: GPL-2.0-or-later

#include "deviceslistmodel.h"
#include "dsomodel.h"
#include "usb/finddevices.h"
#include "usb/uploadFirmware.h"
#include <QColor>
#include <QDebug>

DevicesListModel::DevicesListModel( FindDevices *findDevices, int verboseLevel )
    : findDevices( findDevices ), verboseLevel( verboseLevel ) {}

int DevicesListModel::rowCount( const QModelIndex & ) const { return int( entries.size() ); }

int DevicesListModel::columnCount( const QModelIndex & ) const { return 2; }

QVariant DevicesListModel::headerData( int section, Qt::Orientation orientation, int role ) const {
    if ( orientation == Qt::Vertical )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( role == Qt::DisplayRole ) {
        switch ( section ) {
        case 0:
            return QObject::tr( "Devicename" );
        case 1:
            return QObject::tr( "Status" );
        default:
            return QVariant();
        }
    }
    return QAbstractTableModel::headerData( section, orientation, role );
}

QVariant DevicesListModel::data( const QModelIndex &index, int role ) const {
    if ( !index.isValid() )
        return QVariant();
    const unsigned row = unsigned( index.row() );
    if ( role == Qt::UserRole )
        return QVariant::fromValue( entries[ row ].id );
    if ( role == Qt::UserRole + 1 )
        return QVariant::fromValue( entries[ row ].canConnect );
    if ( role == Qt::UserRole + 2 )
        return QVariant::fromValue( entries[ row ].needFirmware );
    if ( role == Qt::UserRole + 3 )
        return QVariant::fromValue( entries[ row ].errorMessage );

    if ( role == Qt::DisplayRole ) {
        if ( index.column() == 0 ) {
            return entries[ row ].name;
        } else if ( index.column() == 1 ) {
            return entries[ row ].getStatus();
        }
    }

    if ( role == Qt::BackgroundRole ) {
        if ( entries[ row ].canConnect )
            return QColor( Qt::darkGreen ).lighter();
        else if ( entries[ row ].needFirmware )
            return QColor( Qt::yellow ).lighter();
    }

    return QVariant();
}

void DevicesListModel::updateDeviceList() {
    beginResetModel();
    entries.clear();
    endResetModel();
    const FindDevices::DeviceList *devices = findDevices->getDevices();
    beginInsertRows( QModelIndex(), 0, int( devices->size() ) );
    for ( auto &i : *devices ) {
        DeviceListEntry entry;
        entry.name = i.second->getModel()->name;
        entry.id = i.first;
        if ( i.second->needsFirmware() ) {
            if ( verboseLevel > 2 )
                qDebug() << "  DevicesListModel::updateDeviceList()" << entry.name << "upload firmware";
            UploadFirmware uf;
            if ( !uf.startUpload( i.second.get() ) ) {
                entry.errorMessage = uf.getErrorMessage();
                entry.needFirmware = false; // trigger error message
            } else
                entry.needFirmware = true;
        } else if ( i.second->connectDevice( entry.errorMessage ) ) {
            if ( verboseLevel > 2 )
                qDebug() << "  DevicesListModel::updateDeviceList()" << entry.name << "can connect";
            entry.canConnect = true;
            i.second->disconnectFromDevice();
        } else {
            if ( verboseLevel > 2 )
                qDebug() << "  DevicesListModel::updateDeviceList()" << entry.name << "cannot connect";
            entry.canConnect = false;
        }
        entries.push_back( entry );
    }
    endInsertRows();
}
