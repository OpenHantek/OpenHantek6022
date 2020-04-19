// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "rawdevicelistentry.h"
#include "ui_newdevicemodelfromexisting.h"
#include <QDialog>
#include <memory>

namespace Ui {
class NewDeviceModelFromExisting;
}

struct libusb_context;

class NewDeviceModelFromExisting : public QDialog {
    Q_OBJECT

  public:
    explicit NewDeviceModelFromExisting( QWidget *parent = nullptr );
    //     void setUSBcontext(libusb_context* context);
    //     RawDeviceListEntry* getSelectedEntry();
  private:
    std::unique_ptr<Ui::NewDeviceModelFromExisting> ui;
    libusb_context *context = nullptr;

    // QDialog interface
  public slots:
    void accept() override;
};
