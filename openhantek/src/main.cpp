// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QLibraryInfo>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QTranslator>
#include <QVBoxLayout>
#include <iostream>
#include <memory>

#include <libusb-1.0/libusb.h>

#include "dataanalyzer.h"
#include "hantekdsocontrol.h"
#include "mainwindow.h"
#include "settings.h"
#include "usb/finddevices.h"
#include "usb/uploadFirmware.h"
#include "usb/usbdevice.h"

using namespace Hantek;

void showMessage(const QString &message) {
    QMessageBox::information(nullptr, QCoreApplication::translate("", "No connection established!"), message);
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
    if (openHantekTranslator.load(QLocale(), QLatin1String("openhantek"), QLatin1String("_"),
                                  QLatin1String(":/translations"))) {
        openHantekApplication.installTranslator(&openHantekTranslator);
    }

    //////// Find matching usb devices ////////
    libusb_context *context;
    int error = libusb_init(&context);

    if (error) {
        showMessage(QCoreApplication::translate("", "Can't initalize USB: %1").arg(libUsbErrorString(error)));
        return -1;
    }

    FindDevices findDevices;
    std::list<std::unique_ptr<USBDevice>> devices = findDevices.findDevices();

    if (devices.empty()) {
        showMessage(QCoreApplication::translate("", "No Hantek oscilloscope found. Please check if your "
                                                    "device is supported by this software, is connected, "
                                                    "in the right mode (oscilloscope mode) and if the "
                                                    "driver is correctly installed. Refer to the <a "
                                                    "href='https://github.com/OpenHantek/openhantek/"
                                                    "'>website</a> for help: %1")
                        .arg(findDevices.getErrorMessage()));
        return -1;
    }

    //////// Upload firmwares for all connected devices ////////
    for (const auto &i : devices) {
        QString modelName = QString::fromStdString(i->getModel().name);
        if (i->needsFirmware()) {
            UploadFirmware uf;
            uf.startUpload(i.get());
        }
    }
    devices.clear();

    //////// Select device - Autoselect if only one device is ready ////////
    std::unique_ptr<QDialog> dialog = std::unique_ptr<QDialog>(new QDialog);
    QListWidget *w = new QListWidget(dialog.get());

    devices = findDevices.findDevices();
    for (auto &i : devices) {
        QString modelName = QString::fromStdString(i->getModel().name);

        if (i->needsFirmware()) {
            w->addItem(QCoreApplication::translate("Firmware upload dialog", "%1: Firmware upload failed").arg(modelName));
            continue;
        }
        QString errorMessage;
        if (i->connectDevice(errorMessage)) {
            w->addItem(QCoreApplication::translate("Firmware upload dialog", "%1: Ready").arg(modelName));
            w->setCurrentRow(w->count()-1);
        } else {
            w->addItem(QCoreApplication::translate("Firmware upload dialog", "%1: %2").arg(modelName).arg(findDevices.getErrorMessage()));
        }
    }

    if (w->currentRow() == -1 || devices.size()>1) {
        QPushButton *btn = new QPushButton(QCoreApplication::translate("", "Connect to first device"), dialog.get());
        dialog->move(QApplication::desktop()->screen()->rect().center() - w->rect().center());
        dialog->setWindowTitle(QCoreApplication::translate("", "Firmware upload"));
        dialog->setLayout(new QVBoxLayout());
        dialog->layout()->addWidget(w);
        dialog->layout()->addWidget(btn);
        btn->connect(btn, &QPushButton::clicked, QCoreApplication::instance(), &QCoreApplication::quit);
        dialog->show();
        openHantekApplication.exec();
        dialog->close();
    }
    int selectedDevice = w->currentRow();
    dialog.reset(nullptr);

    std::unique_ptr<USBDevice> device;
    int indexCounter = 0;
    for (auto &i : devices) {
        if (indexCounter == selectedDevice) {
            device = std::move(i);
            break;
        }
    }
    devices.clear();

    if (device == nullptr || device->needsFirmware() || !device->isConnected()) {
        showMessage(QCoreApplication::translate("", "A device was found, but the "
                                                    "firmware upload seem to have "
                                                    "failed or the connection "
                                                    "could not be established: %1")
                        .arg(findDevices.getErrorMessage()));
        return -1;
    }

    //////// Create DSO control object and move it to a separate thread ////////
    QThread dsoControlThread;
    dsoControlThread.setObjectName("dsoControlThread");
    HantekDsoControl dsoControl(device.get());
    dsoControl.moveToThread(&dsoControlThread);
    QObject::connect(&dsoControlThread, &QThread::started, &dsoControl, &HantekDsoControl::run);
    QObject::connect(&dsoControl, &HantekDsoControl::communicationError, QCoreApplication::instance(),
                     &QCoreApplication::quit);
    QObject::connect(device.get(), &USBDevice::deviceDisconnected, QCoreApplication::instance(),
                     &QCoreApplication::quit);

    //////// Create data analyser object ////////
    QThread dataAnalyzerThread;
    dataAnalyzerThread.setObjectName("dataAnalyzerThread");
    DataAnalyzer dataAnalyser;
    dataAnalyser.setSourceData(&dsoControl.getLastSamples());
    dataAnalyser.moveToThread(&dataAnalyzerThread);
    QObject::connect(&dsoControl, &HantekDsoControl::samplesAvailable, &dataAnalyser, &DataAnalyzer::samplesAvailable);

    //////// Create settings object ////////
    DsoSettings settings;
    settings.setChannelCount(dsoControl.getChannelCount());
    dataAnalyser.applySettings(&settings.scope);

    //////// Create main window ////////
    OpenHantekMainWindow *openHantekMainWindow = new OpenHantekMainWindow(&dsoControl, &dataAnalyser, &settings);
    openHantekMainWindow->show();

    //////// Start DSO thread and go into GUI main loop
    dsoControl.startSampling();
    dataAnalyzerThread.start();
    dsoControlThread.start();
    int res = openHantekApplication.exec();

    //////// Clean up ////////
    dsoControlThread.quit();
    dsoControlThread.wait(10000);

    dataAnalyzerThread.quit();
    dataAnalyzerThread.wait(10000);
    return res;
}
