// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QSurfaceFormat>
#include <QTranslator>

#include <iostream>
#include <libusb-1.0/libusb.h>
#include <memory>

#include "post/graphgenerator.h"
#include "post/mathchannelgenerator.h"
#include "post/postprocessing.h"
#include "post/spectrumgenerator.h"

#include "dsomodel.h"
#include "hantekdsocontrol.h"
#include "mainwindow.h"
#include "selectdevice/selectsupporteddevice.h"
#include "settings.h"
#include "usb/usbdevice.h"
#include "viewconstants.h"

#ifndef VERSION
#error "You need to run the cmake buildsystem!"
#endif

using namespace Hantek;

/// \brief Initialize the device with the current settings.
void applySettingsToDevice(HantekDsoControl *dsoControl, DsoSettingsScope *scope,
                           const Dso::ControlSpecification *spec) {
    bool mathUsed = scope->anyUsed(spec->channels);
    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        dsoControl->setCoupling(channel, scope->coupling(channel, spec));
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
        ptrdiff_t index = std::distance(recLenVec.begin(),
                                        std::find(recLenVec.begin(), recLenVec.end(), scope->horizontal.recordLength));
        dsoControl->setRecordLength(index < 0 ? 1 : (unsigned)index);
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

    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES, true);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

    // Prefer full desktop OpenGL without fixed pipeline
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); // Antia-Aliasing, Multisampling
    QSurfaceFormat::setDefaultFormat(format);
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

    //////// Create settings object ////////
    auto settings = std::unique_ptr<DsoSettings>(new DsoSettings(&device->getModel()->specification));

    //////// Create post processing objects ////////
    QThread postProcessingThread;
    postProcessingThread.setObjectName("postProcessingThread");
    PostProcessing postProcessing(settings->scope.countChannels());

    SpectrumGenerator spectrumGenerator(&settings->scope);
    MathChannelGenerator mathchannelGenerator(&settings->scope, device->getModel()->specification.channels);
    GraphGenerator graphGenerator(&settings->scope, device->getModel()->specification.isSoftwareTriggerDevice);

    postProcessing.registerProcessor(&mathchannelGenerator);
    postProcessing.registerProcessor(&spectrumGenerator);
    postProcessing.registerProcessor(&graphGenerator);

    postProcessing.moveToThread(&postProcessingThread);
    QObject::connect(&dsoControl, &HantekDsoControl::samplesAvailable, &postProcessing, &PostProcessing::input);

    //////// Create main window ////////
    MainWindow openHantekMainWindow(&dsoControl, settings.get());
    QObject::connect(&postProcessing, &PostProcessing::processingFinished, &openHantekMainWindow,
                     &MainWindow::showNewData);
    openHantekMainWindow.show();

    applySettingsToDevice(&dsoControl, &settings->scope, &device->getModel()->specification);

    //////// Start DSO thread and go into GUI main loop
    dsoControl.startSampling();
    postProcessingThread.start();
    dsoControlThread.start();
    int res = openHantekApplication.exec();

    //////// Clean up ////////
    dsoControlThread.quit();
    dsoControlThread.wait(10000);

    postProcessingThread.quit();
    postProcessingThread.wait(10000);
    return res;
}
