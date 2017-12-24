#pragma once

#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QDialog>
#include <QLabel>
#include <memory>
#include "usb/usbdevice.h"

struct libusb_context;

/**
 * Offers the user a selection dialog. If you call any of the -Modal methods,
 * the method will block and show a dialog for selection or for a usb error
 * message. The method returns as soon as the user closes the dialog.
 *
 * An example to get a device:
 * std::unique_ptr<USBDevice> device = SelectDevice().showSelectDeviceModal(context);
 */
class SelectDevice: public QDialog {
public:
    SelectDevice();
    std::unique_ptr<USBDevice> showSelectDeviceModal(libusb_context *&context);
    void showLibUSBFailedDialogModel(int error);
private:
    void makeErrorDialog(const QString& message);
    void updateDeviceList();
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    QTableView *w;
    QLabel *label;
    QPushButton *btn;
    UniqueUSBid selectedDevice = 0;
};
