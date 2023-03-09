// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ui_selectsupporteddevice.h"

#include <QDialog>
#include <QPushButton>

#include "usb/scopedevice.h"
#include <memory>

struct libusb_context;

/**
 * Offers the user a device selection dialog. If you call any of the -Modal methods,
 * the method will block and show a dialog for selection or for a usb error
 * message. The method returns as soon as the user closes the dialog.
 *
 * An example to get a user selected device:
 * std::unique_ptr<USBDevice> device = SelectDevice().showSelectDeviceModal(context, ...);
 */
class SelectSupportedDevice : public QDialog {
    Q_OBJECT

  public:
    explicit SelectSupportedDevice( QWidget *parent = nullptr );
    std::unique_ptr< ScopeDevice > showSelectDeviceModal( libusb_context *context, int verboseLevel = 0, bool autoConnect = true );
    void showLibUSBFailedDialogModel( int error );

  private:
    void updateDeviceList();
    void updateSupportedDevices();
    std::unique_ptr< Ui::SelectSupportedDevice > ui;
    UniqueUSBid selectedDevice = 0;
    bool demoModeClicked = false;
    int verboseLevel = 0;
    QPushButton *btnDemoMode;
};
