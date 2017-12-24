#include "deviceslistmodel.h"
#include "usb/finddevices.h"
#include "usb/uploadFirmware.h"
#include "dsomodel.h"
#include <QColor>

DevicesListModel::DevicesListModel(FindDevices *findDevices) :findDevices(findDevices) {}

int DevicesListModel::rowCount(const QModelIndex &) const
{
    return entries.size();
}

int DevicesListModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant DevicesListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QAbstractTableModel::headerData(section, orientation, role);
    if (role == Qt::DisplayRole) {
        switch(section) {
        case 0: return tr("Devicename");
        case 1: return tr("Status");
        default: return QVariant();
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant DevicesListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (role==Qt::UserRole) return QVariant::fromValue(entries[index.row()].id);
    else if (role==Qt::UserRole+1) return QVariant::fromValue(entries[index.row()].canConnect);
    else if (role==Qt::UserRole+2) return QVariant::fromValue(entries[index.row()].needFirmware);

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return entries[index.row()].name;
        } else if (index.column() == 1) {
            return entries[index.row()].getStatus();
        }
    }

    if (role == Qt::BackgroundRole) {
        if (entries[index.row()].canConnect) return QColor(Qt::darkGreen).lighter();
        else if (entries[index.row()].needFirmware) return QColor(Qt::yellow).lighter();
    }

    return QVariant();
}

void DevicesListModel::updateDeviceList()
{
    beginResetModel();
    entries.clear();
    endResetModel();
    const FindDevices::DeviceList* devices = findDevices->getDevices();
    beginInsertRows(QModelIndex(),0,devices->size());
    for (auto &i : *devices) {
        DeviceListEntry entry;
        entry.name= QString::fromStdString(i.second->getModel()->name);
        entry.id = i.first;
        if (i.second->needsFirmware()) {
            UploadFirmware uf;
            if (!uf.startUpload(i.second.get())) {
                entry.errorMessage = uf.getErrorMessage();
            }
            entry.needFirmware = true;
        } else if (i.second->connectDevice(entry.errorMessage)) {
            entry.canConnect = true;
            i.second->disconnectFromDevice();
        } else {
            entry.canConnect = false;
        }
        entries.push_back(entry);
    }
    endInsertRows();
}
