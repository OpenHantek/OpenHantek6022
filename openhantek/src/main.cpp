// SPDX-License-Identifier: GPL-2.0+

#include <QDebug>
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include <iostream>
#include <memory>
#include <libusb-1.0/libusb.h>

#include "analyse/dataanalyzer.h"
#include "hantekdsocontrol.h"
#include "mainwindow.h"
#include "settings.h"
#include "usb/usbdevice.h"
#include "dsomodel.h"
#include "selectdevice/selectsupporteddevice.h"

using namespace Hantek;

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
    libusb_context *context = nullptr;
    int error = libusb_init(&context);
    if (error) {
        SelectSupportedDevice().showLibUSBFailedDialogModel(error);
        return -1;
    }
    std::unique_ptr<USBDevice> device = SelectSupportedDevice().showSelectDeviceModal(context);

    QString errorMessage;
    if (device == nullptr || !device->connectDevice(errorMessage)) {
        libusb_exit(context);
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
    DsoSettings settings(dsoControl.getDeviceSettings(), &device->getModel()->specification);
    dataAnalyser.applySettings(&settings);

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
