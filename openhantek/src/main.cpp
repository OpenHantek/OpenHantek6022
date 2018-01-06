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
#include "viewconstants.h"

using namespace Hantek;


/// \brief Initialize the device with the current settings.
void applySettingsToDevice(HantekDsoControl* dsoControl, DsoSettingsScope* scope, const Dso::ControlSpecification* spec) {
    bool mathUsed = scope->anyUsed(spec->channels);
    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        dsoControl->setCoupling(channel, scope->coupling(channel,spec));
        dsoControl->setGain(channel, scope->gain(channel) * DIVS_VOLTAGE);
        dsoControl->setOffset(channel, (scope->voltage[channel].offset / DIVS_VOLTAGE) + 0.5);
        dsoControl->setTriggerLevel(channel, scope->voltage[channel].trigger);
        dsoControl->setChannelUsed(channel, mathUsed | scope->anyUsed(channel));
    }

    if (scope->horizontal.samplerateSource == DsoSettingsScopeHorizontal::Samplerrate)
        dsoControl->setSamplerate(scope->horizontal.samplerate);
    else
        dsoControl->setRecordTime(scope->horizontal.timebase * DIVS_TIME);

    if (dsoControl->getAvailableRecordLengths().empty())
        dsoControl->setRecordLength(scope->horizontal.recordLength);
    else {
        auto recLenVec = dsoControl->getAvailableRecordLengths();
        ptrdiff_t index = std::distance(
            recLenVec.begin(), std::find(recLenVec.begin(), recLenVec.end(), scope->horizontal.recordLength));
        dsoControl->setRecordLength(index < 0 ? 1 : index);
    }
    dsoControl->setTriggerMode(scope->trigger.mode);
    dsoControl->setPretriggerPosition(scope->trigger.position * scope->horizontal.timebase * DIVS_TIME);
    dsoControl->setTriggerSlope(scope->trigger.slope);
    dsoControl->setTriggerSource(scope->trigger.special, scope->trigger.source);
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
    DsoSettings settings(&device->getModel()->specification);
    dataAnalyser.applySettings(&settings);

    //////// Create main window ////////
    MainWindow *openHantekMainWindow = new MainWindow(&dsoControl, &dataAnalyser, &settings);
    openHantekMainWindow->show();

    applySettingsToDevice(&dsoControl,&settings.scope,&device->getModel()->specification);

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
