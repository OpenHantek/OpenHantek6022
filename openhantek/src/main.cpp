// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLibraryInfo>
#include <QLocale>
#include <QDebug>
#include <QMessageBox>
#include <QTranslator>
#include <QTimer>
#include <QListWidget>
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
    //////// Set application information ////////
    QCoreApplication::setOrganizationName("OpenHantek");
    QCoreApplication::setOrganizationDomain("www.openhantek.org");
    QCoreApplication::setApplicationName("OpenHantek");
    QCoreApplication::setApplicationVersion(VERSION);

    QApplication openHantekApplication(argc, argv);

    //////// Load translations ////////
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        openHantekApplication.installTranslator(&qtTranslator);

    QTranslator openHantekTranslator;
    if (openHantekTranslator.load(QLocale(), QLatin1String("openhantek"),
                                  QLatin1String("_"),
                                  QLatin1String(":/translations"))) {
        openHantekApplication.installTranslator(&openHantekTranslator);
    }

    //////// Find matching usb devices ////////
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

    //////// Upload firmwares for all connected devices ////////
    std::unique_ptr<QDialog> dialog = std::unique_ptr<QDialog>(new QDialog);
    QListWidget* w = new QListWidget(dialog.get());
    QPushButton* btn = new QPushButton(QCoreApplication::translate("","Connect to first device"),dialog.get());
    dialog->move(QApplication::desktop()->screen()->rect().center() - w->rect().center());
    dialog->setWindowTitle(QCoreApplication::translate("","Firmware upload"));
    for (const auto& i: devices) {
        QString modelName = QString::fromStdString(i->getModel().name);
        if (i->needsFirmware()) {
            UploadFirmware uf;
            if (!uf.startUpload(i.get())) {
                w->addItem(QCoreApplication::translate("Firmware upload dialog, failed","%1: Failed (%2)").arg(modelName).arg(uf.getErrorMessage()));
            } else {
                w->addItem(QCoreApplication::translate("Firmware upload dialog, success","%1: Uploaded").arg(modelName));
            }
        } else {
            w->addItem(QCoreApplication::translate("Firmware upload dialog, success","%1: Ready").arg(modelName));
        }
    }
    devices.clear();
    dialog->setLayout(new QVBoxLayout());
    dialog->layout()->addWidget(w);
    dialog->layout()->addWidget(btn);
    btn->connect(btn, &QPushButton::clicked, QCoreApplication::instance(), &QCoreApplication::quit);
    dialog->show();
    openHantekApplication.exec();
    dialog->close();
    dialog.reset(nullptr);

    //////// Find first ready device ////////
    devices = findDevices.findDevices();
    std::unique_ptr<USBDevice> device;
    for (auto& i: devices) {
        if (i->needsFirmware()) continue;
        QString modelName = QString::fromStdString(i->getModel().name);
        QString errorMessage;
        if (i->connectDevice(errorMessage)) {
            device = std::move(i);
            break;
        } else {
            showMessage(QCoreApplication::translate("","The connection to %1 can not be established: %2").arg(modelName).arg(findDevices.getErrorMessage()));
        }
    }

    if (device == nullptr) {
        showMessage(QCoreApplication::translate("","A device was found, but the firmware upload seem to have failed or the connection could not be established: %1").arg(findDevices.getErrorMessage()));
        return -1;
    }

    //////// Create DSO control object and move it to a separate thread ////////
    QThread dsoControlThread;
    std::shared_ptr<HantekDsoControl> dsoControl(new HantekDsoControl(device.get()));
    dsoControl->moveToThread(&dsoControlThread);
    QObject::connect(&dsoControlThread,&QThread::started,dsoControl.get(),&HantekDsoControl::run);
    QObject::connect(dsoControl.get(), &HantekDsoControl::communicationError, QCoreApplication::instance(), &QCoreApplication::quit);
    QObject::connect(device.get(), &USBDevice::deviceDisconnected, QCoreApplication::instance(), &QCoreApplication::quit);

    //////// Create data analyser object ////////
    std::shared_ptr<DataAnalyzer> dataAnalyser(new DataAnalyzer());

    //////// Create main window ////////
    OpenHantekMainWindow *openHantekMainWindow = new OpenHantekMainWindow(dsoControl, dataAnalyser);
    openHantekMainWindow->show();

    //////// Start DSO thread and go into GUI main loop
    dsoControlThread.start();
    int res = openHantekApplication.exec();

    //////// Clean up ////////
    dsoControlThread.quit();
    dsoControlThread.wait(10000);
    return res;
}
