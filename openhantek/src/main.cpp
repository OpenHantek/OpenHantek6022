// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QDebug>
#include <QMessageBox>
#include <QTranslator>
#include <QTimer>
#include <iostream>
#include <memory>

#include <libusb-1.0/libusb.h>

#include "hantekdsocontrol.h"
#include "dataanalyzer.h"
#include "mainwindow.h"
#include "usb/finddevices.h"
#include "usb/usbdevice.h"
#include "usb/uploadFirmware.h"

using namespace Hantek;

void showMessage(const QString& message) {
    QMessageBox::information(nullptr,QCoreApplication::translate("","No connection established!"), message);
}

/// \brief Initialize resources and translations and show the main window.
int main(int argc, char *argv[]) {
  // Set application information
  QCoreApplication::setOrganizationName("OpenHantek");
  QCoreApplication::setOrganizationDomain("www.openhantek.org");
  QCoreApplication::setApplicationName("OpenHantek");
  QCoreApplication::setApplicationVersion(VERSION);

  QApplication openHantekApplication(argc, argv);

  QTranslator qtTranslator;
  if (qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    openHantekApplication.installTranslator(&qtTranslator);

  QTranslator openHantekTranslator;
  if (openHantekTranslator.load(QLocale(), QLatin1String("openhantek"),
                                QLatin1String("_"),
                                QLatin1String(":/translations"))) {
      openHantekApplication.installTranslator(&openHantekTranslator);
  }

  libusb_context* context;
  int error = libusb_init(&context);

  if (error){
      showMessage(QCoreApplication::translate("","Can't initalize USB: %1").arg(libUsbErrorString(error)));
      return -1;
  }

  FindDevices findDevices;
  std::list<std::unique_ptr<USBDevice>> devices = findDevices.findDevices();

  if (devices.empty()) {
      showMessage(QCoreApplication::translate("","No Hantek oscilloscope found. Please check if your device is supported by this software, is connected, in the right mode (oscilloscope mode) and if the driver is correctly installed. Refer to the <a href='https://github.com/OpenHantek/openhantek/'>website</a> for help: %1").arg(findDevices.getErrorMessage()));
      return -1;
  }

  // Upload firmwares for all connected devices
  for (auto i = devices.begin(); i != devices.end(); ++i) {
      if (i->get()->needsFirmware()) {
          UploadFirmware uf;
          if (!uf.startUpload(i->get())) {
            showMessage(QCoreApplication::translate("","Uploading firmware to %1: failed").arg(uf.getErrorMessage()));
          } else {
              showMessage(QCoreApplication::translate("","Uploading firmware to %1: done").arg(QString::fromStdString(i->get()->getModel().name)));
          }
      }
  }

  devices.clear();

  // Find first ready device
  devices = findDevices.findDevices();
  std::unique_ptr<USBDevice> device;
  for (auto i = devices.begin(); i != devices.end(); ++i) {
      if (i->get()->needsFirmware()) continue;
      QString errorMessage;
      if (i->get()->connectDevice(errorMessage)) {
          device = std::move(*i);
          break;
      } else {
        showMessage(QCoreApplication::translate("","A device was found, but the connection could not be established: %1").arg(findDevices.getErrorMessage()));
      }
  }

  if (device == nullptr) {
      showMessage(QCoreApplication::translate("","A device was found, but the firmware upload seem to have failed or the connection could not be established: %1").arg(findDevices.getErrorMessage()));
      return -1;
  }

  // USB device
  QObject::connect(device.get(), &USBDevice::deviceDisconnected, QCoreApplication::instance(), &QCoreApplication::quit);

  QThread dsoControlThread;
  std::shared_ptr<HantekDsoControl> dsoControl(new HantekDsoControl(device.get()));
  dsoControl->moveToThread(&dsoControlThread);
  QObject::connect(&dsoControlThread,&QThread::started,dsoControl.get(),&HantekDsoControl::run);
  QObject::connect(dsoControl.get(), &HantekDsoControl::communicationError, QCoreApplication::instance(), &QCoreApplication::quit);

  std::shared_ptr<DataAnalyzer> dataAnalyser(new DataAnalyzer());

  OpenHantekMainWindow *openHantekMainWindow = new OpenHantekMainWindow(dsoControl, dataAnalyser);
  openHantekMainWindow->show();

  dsoControlThread.start();

  int res = openHantekApplication.exec();

  dsoControlThread.quit();
  dsoControlThread.wait(10000);
  return res;
}
