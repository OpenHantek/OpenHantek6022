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
#include <sched.h>
#endif
#include <iostream>
#ifdef __FreeBSD__
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#include <memory>

// Settings
#include "dsosettings.h"
#include "viewconstants.h"
#include "viewsettings.h"

// DSO core logic
#include "capturing.h"
#include "dsomodel.h"
#include "hantekdsocontrol.h"
#include "usb/scopedevice.h"

// Post processing
#include "post/graphgenerator.h"
#include "post/mathchannelgenerator.h"
#include "post/postprocessing.h"
#include "post/spectrumgenerator.h"

// Exporter
#include "exporting/exportcsv.h"
#include "exporting/exporterprocessor.h"
#include "exporting/exporterregistry.h"
// legacy img and pdf export is replaced by MainWindow::screenshot()
#ifdef LEGACYEXPORT
#include "exporting/exportimage.h"
#include "exporting/exportprint.h"
#endif

// GUI
#include "iconfont/QtAwesome.h"
#include "mainwindow.h"
#include "selectdevice/selectsupporteddevice.h"

// OpenGL setup
#include "glscope.h"

#include "models/modelDEMO.h"

#ifndef VERSION
#error "You need to run the cmake buildsystem!"
#endif
#include "OH_VERSION.h"


using namespace Hantek;


#if 0
class OHApplication Q_DECL_FINAL : public QApplication {
    // Q_OBJECT
  public:
    OHApplication( int &argc, char **argv ) : QApplication( argc, argv ) {}

    bool notify( QObject *receiver, QEvent *event ) Q_DECL_OVERRIDE {
        try {
            return QApplication::notify( receiver, event );
            //} catch (Tango::DevFailed &e) {
            // Handle the desired exception type
        } catch ( ... ) {
            qDebug() << "notify:" << event;
            exit( 0 );
            // Handle the rest
        }

        return false;
    }
};
#endif

/// \brief Initialize resources and translations and show the main window.
int main( int argc, char *argv[] ) {
    //////// Set application information ////////
    QCoreApplication::setOrganizationName( "OpenHantek" );
    QCoreApplication::setOrganizationDomain( "openhantek.org" );
    QCoreApplication::setApplicationName( "OpenHantek6022" );
    QCoreApplication::setApplicationVersion( VERSION );
    QCoreApplication::setAttribute( Qt::AA_UseHighDpiPixmaps, true );
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 ) )
    QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling, true );
#endif
    bool demoMode = false;
    bool useGLES = false;
    bool useLocale = true;
    {
        QCoreApplication parserApp( argc, argv );
        QCommandLineParser p;
        p.addHelpOption();
        p.addVersionOption();
        QCommandLineOption demoModeOption( {"d", "demoMode"}, "Demo mode without scope HW" );
        p.addOption( demoModeOption );
        QCommandLineOption useGlesOption( {"e", "useGLES"}, "Use OpenGL ES instead of OpenGL" );
        p.addOption( useGlesOption );
        QCommandLineOption intOption( {"i", "international"}, "Show the international interface, do not translate" );
        p.addOption( intOption );
        p.process( parserApp );
        demoMode = p.isSet( demoModeOption );
        useGLES = p.isSet( useGlesOption );
        useLocale = !p.isSet( intOption );
    }

#ifdef __arm__
    // HACK: Raspberry Pi crashes with OpenGL, use always OpenGLES
    useGLES = true;
#endif

    GlScope::fixOpenGLversion( useGLES ? QSurfaceFormat::OpenGLES : QSurfaceFormat::OpenGL );

    QApplication openHantekApplication( argc, argv );

#ifdef __linux__
    // Qt5 linux default
    // ("Breeze", "Windows", "Fusion")
    // with package qt5-style-plugins
    // ("Breeze", "bb10dark", "bb10bright", "cleanlooks", "gtk2", "cde", "motif", "plastique", "Windows", "Fusion")
    openHantekApplication.setStyle( QStyleFactory::create( "Fusion" ) ); // smaller widgets allow stacking of all docks
#endif

#ifdef __linux_rt__
    // try to set realtime priority to improve USB allocation
    // this works if the user is member of a realtime group, e.g. audio:
    // 1. set limits in /etc/security/limits.d:
    //    @audio - rtprio 99
    // 2. add user to the group, e.g. audio:
    //    usermod -a -G audio <your_user_name>
    // or set the limits only for your user in /etc/security/limits.d:
    //    <your_user_name> - rtprio 99
    struct sched_param schedParam;
    schedParam.sched_priority = 49;                   // set RT priority level 50
    sched_setscheduler( 0, SCHED_FIFO, &schedParam ); // and RT FIFO scheduler
    // but ignore any error if user has no realtime rights
#endif

    //////// Load translations ////////
    QTranslator qtTranslator;
    QTranslator openHantekTranslator;
    if ( useLocale && QLocale::system().name() != "en_US" ) { // somehow Qt on MacOS uses the german translation for en_US?!
        if ( qtTranslator.load( "qt_" + QLocale::system().name(), QLibraryInfo::location( QLibraryInfo::TranslationsPath ) ) ) {
            openHantekApplication.installTranslator( &qtTranslator );
        }
        if ( openHantekTranslator.load( QLocale(), QLatin1String( "openhantek" ), QLatin1String( "_" ),
                                        QLatin1String( ":/translations" ) ) ) {
            openHantekApplication.installTranslator( &openHantekTranslator );
        }
    }

    //////// Find matching usb devices ////////
    libusb_context *context = nullptr;

    std::unique_ptr< ScopeDevice > scopeDevice = nullptr;

    if ( !demoMode ) {
        int error = libusb_init( &context );
        if ( error ) {
            SelectSupportedDevice().showLibUSBFailedDialogModel( error );
            return -1;
        }
        // SelectSupportedDevive returns a real device unless demoMode is true
        scopeDevice = SelectSupportedDevice().showSelectDeviceModal( context );
        if ( scopeDevice && scopeDevice->isDemoDevice() ) {
            demoMode = true;
            libusb_exit( context ); // stop all USB activities
            context = nullptr;
        } else {
            QString errorMessage;
            if ( scopeDevice == nullptr || !scopeDevice->connectDevice( errorMessage ) ) {
                libusb_exit( context ); // clean USB
                if ( !errorMessage.isEmpty() )
                    qCritical() << errorMessage;
                return -1;
            }
        }
    } else {
        scopeDevice = std::unique_ptr< ScopeDevice >( new ScopeDevice() );
    }

    // Here we have either a connected scope device or a demo device w/o hardware
    const DSOModel *model = scopeDevice->getModel();

    //////// Create DSO control object and move it to a separate thread ////////
    QThread dsoControlThread;
    dsoControlThread.setObjectName( "dsoControlThread" );
    HantekDsoControl dsoControl( scopeDevice ? scopeDevice.get() : nullptr, model );
    dsoControl.moveToThread( &dsoControlThread );
    QObject::connect( &dsoControlThread, &QThread::started, &dsoControl, &HantekDsoControl::stateMachine );
    QObject::connect( &dsoControl, &HantekDsoControl::communicationError, QCoreApplication::instance(), &QCoreApplication::quit );

    if ( scopeDevice )
        QObject::connect( scopeDevice.get(), &ScopeDevice::deviceDisconnected, QCoreApplication::instance(),
                          &QCoreApplication::quit );

    const Dso::ControlSpecification *spec = model->spec();

    //////// Create settings object ////////
    DsoSettings settings( spec );

    //////// Create exporters ////////
    ExporterRegistry exportRegistry( spec, &settings );

    ExporterCSV exporterCSV;
    // ExporterImage exportImage;
    // ExporterPrint exportPrint;

    ExporterProcessor samplesToExportRaw( &exportRegistry );

    exportRegistry.registerExporter( &exporterCSV );
#ifdef LEGACYEXPORT
    exportRegistry.registerExporter( &exportImage );
    exportRegistry.registerExporter( &exportPrint );
#endif
    //////// Create post processing objects ////////
    QThread postProcessingThread;
    postProcessingThread.setObjectName( "postProcessingThread" );
    PostProcessing postProcessing( settings.scope.countChannels() );

    SpectrumGenerator spectrumGenerator( &settings.scope, &settings.post );
    MathChannelGenerator mathchannelGenerator( &settings.scope, spec->channels );
    GraphGenerator graphGenerator( &settings.scope );

    postProcessing.registerProcessor( &samplesToExportRaw );
    postProcessing.registerProcessor( &mathchannelGenerator );
    postProcessing.registerProcessor( &spectrumGenerator );
    postProcessing.registerProcessor( &graphGenerator );

    postProcessing.moveToThread( &postProcessingThread );
    QObject::connect( &dsoControl, &HantekDsoControl::samplesAvailable, &postProcessing, &PostProcessing::input );
    QObject::connect( &postProcessing, &PostProcessing::processingFinished, &exportRegistry, &ExporterRegistry::input,
                      Qt::DirectConnection );

    //////// Create main window ////////
    iconFont->initFontAwesome();
    MainWindow openHantekMainWindow( &dsoControl, &settings, &exportRegistry );
    QObject::connect( &postProcessing, &PostProcessing::processingFinished, &openHantekMainWindow, &MainWindow::showNewData );
    QObject::connect( &exportRegistry, &ExporterRegistry::exporterProgressChanged, &openHantekMainWindow,
                      &MainWindow::exporterProgressChanged );
    QObject::connect( &exportRegistry, &ExporterRegistry::exporterStatusChanged, &openHantekMainWindow,
                      &MainWindow::exporterStatusChanged );
    openHantekMainWindow.show();

    //////// Start DSO thread and go into GUI main loop
    dsoControl.enableSampling( true );
    postProcessingThread.start();
    dsoControlThread.start();
    Capturing capturing( &dsoControl );
    capturing.start();

    int appStatus = openHantekApplication.exec();

    //////// Application closed, clean up step by step ////////

    std::cout << std::unitbuf; // enable automatic flushing

    dsoControl.stopSampling(); // send USB control command
    dsoControl.enableSampling( false );

    unsigned waitForCapturing = unsigned( 2000 * dsoControl.getSamplesize() / dsoControl.getSamplerate() );
    if ( waitForCapturing < 10000 ) // minimum 10 s
        waitForCapturing = 10000;
    capturing.requestInterruption();
    capturing.wait( waitForCapturing );
    // capturing.terminate();
    std::cout << "OpenHantek6022 ";

    // first stop the data acquisition
    // wait 2 * record time (delay is ms) for dso to finish
    unsigned waitForDso = unsigned( 2000 * dsoControl.getSamplesize() / dsoControl.getSamplerate() );
    if ( waitForDso < 10000 ) // minimum 10 s
        waitForDso = 10000;
    // dsoControlThread.terminate();
    dsoControlThread.quit();
    dsoControlThread.wait( waitForDso );
    std::cout << "stopped ";

    // next stop the data processing
    // postProcessingThread.terminate();
    postProcessingThread.quit();
    postProcessingThread.wait( 10000 );
    std::cout << "after ";

    // finally stop the USB communication
    dsoControl.stopSampling(); // send USB control command
    if ( scopeDevice )
        scopeDevice.reset(); // destroys unique_pointer, causes libusb_close(), must be called before libusb_exit()
    if ( context )
        libusb_exit( context );
    std::cout << openHantekMainWindow.elapsedTime.elapsed() / 1000 << " s\n";

    return appStatus;
}
