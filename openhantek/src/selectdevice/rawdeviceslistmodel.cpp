#include "rawdeviceslistmodel.h"
#include "usb/finddevices.h"
#include "usb/uploadFirmware.h"
#include "dsomodel.h"
#include <QColor>

RawDevicesListModel::RawDevicesListModel(libusb_context *context, QObject *parent) : QAbstractTableModel(parent), context(context) {}

int RawDevicesListModel::rowCount(const QModelIndex &) const
{
    return (int)entries.size();
}

int RawDevicesListModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant RawDevicesListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const unsigned row = (unsigned)index.row();
    if (role==ProductIDRole) return QVariant::fromValue(entries[row].productId);
    else if (role==VendorIDRole) return QVariant::fromValue(entries[row].vendorId);
    else if (role==AccessRole) return QVariant::fromValue(entries[row].access);
    else if (role==DeviceNameRole) return QVariant::fromValue(entries[row].devicename);
    else if (role==EntryPointerRole) return QVariant::fromValue((void*)&entries[row]);
    else if (role==Qt::DisplayRole) return QVariant::fromValue(entries[row].deviceinfo);

    return QVariant();
}

QString readUSBdescriptor(libusb_device_handle *handle, uint8_t index) {
    unsigned char string[255];
    int ret = libusb_get_string_descriptor_ascii(handle, index, string, sizeof(string));
    if (ret > 0)
        return QString::fromLatin1((char*)string, ret).trimmed();
    else
        return QString();
}

void RawDevicesListModel::updateDeviceList()
{
    beginResetModel();
    entries.clear();
    endResetModel();

    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list(context, &deviceList);
    beginInsertRows(QModelIndex(),0,(int)deviceCount);

    for (ssize_t deviceIterator = 0; deviceIterator < deviceCount; ++deviceIterator) {
        libusb_device *device = deviceList[deviceIterator];
        RawDeviceListEntry entry;
        // Get device descriptor
        struct libusb_device_descriptor descriptor;
        libusb_get_device_descriptor(device, &descriptor);

        entry.productId = descriptor.idProduct;
        entry.vendorId = descriptor.idVendor;
        libusb_device_handle *handle = NULL;
        int ret = libusb_open(device, &handle);
        if (ret != LIBUSB_SUCCESS) {
            entry.access = false;
            entry.deviceinfo = tr("%1:%2 - No access").arg(entry.vendorId,0,16).arg(entry.productId,0,16);
        } else {
            entry.access = true;
            entry.devicename = readUSBdescriptor(handle, descriptor.iProduct);
            entry.deviceinfo = tr("%1:%2 (%3 - %4)").arg(entry.vendorId,0,16).arg(entry.productId,0,16)
                    .arg(entry.devicename).arg(readUSBdescriptor(handle, descriptor.iManufacturer));
            libusb_close(handle);
        }

        entries.push_back(entry);
    }

    libusb_free_device_list(deviceList, true);

    endInsertRows();
}
