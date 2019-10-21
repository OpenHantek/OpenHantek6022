// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QSurfaceFormat>
#include <QTranslator>
#ifdef __linux__
#include <QStyleFactory>
#endif
#include <iostream>
#ifdef __FreeBSD__
	#include <libusb.h>
#else
	#include <libusb-1.0/libusb.h>
#endif
#include <memory>

// Settings
#include "settings.h"
#include "viewsettings.h"
#include "viewconstants.h"

// DSO core logic
#include "dsomodel.h"
#include "hantekdsocontrol.h"
#include "usb/usbdevice.h"

// Post processing
#include "post/graphgenerator.h"
#include "post/mathchannelgenerator.h"
#include "post/postprocessing.h"
#include "post/spectrumgenerator.h"

// Exporter
#include "exporting/exportcsv.h"
#include "exporting/exporterprocessor.h"
#include "exporting/exporterregistry.h"
#include "exporting/exportimage.h"
#include "exporting/exportprint.h"

// GUI
#include "iconfont/QtAwesome.h"
#include "mainwindow.h"
#include "selectdevice/selectsupporteddevice.h"

// OpenGL setup
#include "glscope.h"

#ifndef VERSION
#error "You need to run the cmake buildsystem!"
#endif
#include "OH_VERSION.h"


using namespace Hantek;

/// \brief Initialize the device with the current settings.
void applySettingsToDevice(HantekDsoControl *dsoControl, DsoSettingsScope *scope,
                           const Dso::ControlSpecification *spec) {
    bool mathUsed = scope->anyUsed(spec->channels);
    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        dsoControl->setProbe( channel, scope->voltage[channel].probeUsed, scope->voltage[channel].probeAttn );
        dsoControl->setGain(channel, scope->gain(channel) * DIVS_VOLTAGE);
        dsoControl->setTriggerLevel(channel, scope->voltage[channel].trigger);
        dsoControl->setChannelUsed(channel, mathUsed | scope->anyUsed(channel));
        dsoControl->setChannelInverted(channel, scope->voltage[channel].inverted);
        dsoControl->setCoupling(channel, Dso::Coupling(scope->voltage[channel].couplingOrMathIndex));
    }

    dsoControl->setRecordTime(scope->horizontal.timebase * DIVS_TIME);
    dsoControl->setTriggerMode(scope->trigger.mode);
    dsoControl->setTriggerPosition(scope->trigger.position);
    dsoControl->setTriggerSlope(scope->trigger.slope);
    dsoControl->setTriggerSource(scope->trigger.source, scope->trigger.smooth);
}

/// \brief Initialize resources and translations and show the main window.
int main(int argc, char *argv[]) {
    //////// Set application information ////////
    QCoreApplication::setOrganizationName("OpenHantek");
    QCoreApplication::setOrganizationDomain("www.openhantek.org");
    QCoreApplication::setApplicationName("OpenHantek6022");
    QCoreApplication::setApplicationVersion(VERSION);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif

    bool useGLES = false;
    {
        QCoreApplication parserApp(argc, argv);
        QCommandLineParser p;
        p.addHelpOption();
        p.addVersionOption();
        QCommandLineOption useGlesOption("useGLES", QCoreApplication::tr("Use OpenGL ES instead of OpenGL"));
        p.addOption(useGlesOption);
        p.process(parserApp);
        useGLES = p.isSet(useGlesOption);
    }

#ifdef __arm__
    // HACK: Raspberry Pi crashes with OpenGL, use always OpenGLES
    useGLES = true;
#endif

    GlScope::fixOpenGLversion( useGLES ? QSurfaceFormat::OpenGLES : QSurfaceFormat::OpenGL );

    QApplication openHantekApplication(argc, argv);

#ifdef __linux__
    // Qt5 linux default
    // ("Breeze", "Windows", "Fusion")
    // with package qt5-style-plugins
    // ("Breeze", "bb10dark", "bb10bright", "cleanlooks", "gtk2", "cde", "motif", "plastique", "Windows", "Fusion")
    openHantekApplication.setStyle( QStyleFactory::create( "Fusion" ) ); // smaller widgets allow stacking of all docks
#endif

    //////// Load translations ////////
    QTranslator qtTranslator;
    QTranslator openHantekTranslator;
    if (QLocale::system().name() != "en_US") { // somehow Qt on MacOS uses the german translation for en_US?!
        if (qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
           openHantekApplication.installTranslator(&qtTranslator);
        }
        if (openHantekTranslator.load(QLocale(), QLatin1String("openhantek"), QLatin1String("_"),
                                      QLatin1String(":/translations"))) {
            openHantekApplication.installTranslator(&openHantekTranslator);
        }
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
    DsoSettings settings(device->getModel()->spec());

    //////// Create exporters ////////
    ExporterRegistry exportRegistry(device->getModel()->spec(), &settings);

    ExporterCSV exporterCSV;
    ExporterImage exportImage;
    ExporterPrint exportPrint;

    ExporterProcessor samplesToExportRaw(&exportRegistry);

    exportRegistry.registerExporter(&exporterCSV);
    exportRegistry.registerExporter(&exportImage);
    exportRegistry.registerExporter(&exportPrint);

    //////// Create post processing objects ////////
    QThread postProcessingThread;
    postProcessingThread.setObjectName("postProcessingThread");
    PostProcessing postProcessing(settings.scope.countChannels());

    SpectrumGenerator spectrumGenerator(&settings.scope, &settings.post);
    MathChannelGenerator mathchannelGenerator(&settings.scope, device->getModel()->spec()->channels);
    GraphGenerator graphGenerator(&settings.scope, &settings.view);

    postProcessing.registerProcessor(&samplesToExportRaw);
    postProcessing.registerProcessor(&mathchannelGenerator);
    postProcessing.registerProcessor(&spectrumGenerator);
    postProcessing.registerProcessor(&graphGenerator);

    postProcessing.moveToThread(&postProcessingThread);
    QObject::connect(&dsoControl, &HantekDsoControl::samplesAvailable, &postProcessing, &PostProcessing::input);
    QObject::connect(&postProcessing, &PostProcessing::processingFinished, &exportRegistry, &ExporterRegistry::input,
                     Qt::DirectConnection);

    //////// Create main window ////////
    iconFont->initFontAwesome();
    MainWindow openHantekMainWindow(&dsoControl, &settings, &exportRegistry);
    QObject::connect(&postProcessing, &PostProcessing::processingFinished, &openHantekMainWindow,
                     &MainWindow::showNewData);
    QObject::connect(&exportRegistry, &ExporterRegistry::exporterProgressChanged, &openHantekMainWindow,
                     &MainWindow::exporterProgressChanged);
    QObject::connect(&exportRegistry, &ExporterRegistry::exporterStatusChanged, &openHantekMainWindow,
                     &MainWindow::exporterStatusChanged);
    openHantekMainWindow.show();

    applySettingsToDevice(&dsoControl, &settings.scope, device->getModel()->spec());

    //////// Start DSO thread and go into GUI main loop
    dsoControl.enableSampling(true);
    postProcessingThread.start();
    dsoControlThread.start();
    int res = openHantekApplication.exec();

    //////// Clean up ////////

    // wait 2 * record time (delay is ms) for dso to finish
    unsigned waitForDso = 2000 * dsoControl.getSamplesize() / dsoControl.getSamplerate();
    if ( waitForDso < 10000 ) // minimum 10 s
        waitForDso = 10000;
    dsoControlThread.quit();
    dsoControlThread.wait( waitForDso );

    postProcessingThread.quit();
    postProcessingThread.wait(10000);

    if (context && device != nullptr) { 
        device.reset(); // causes libusb_close(), which must be called before libusb_exit() 
        libusb_exit(context); 
    }

    return res;
}
