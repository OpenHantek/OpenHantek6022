// SPDX-License-Identifier: GPL-2.0+

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDesktopWidget>
#include <QLibraryInfo>
#include <QLocale>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QTranslator>
#ifdef Q_OS_LINUX
#include <sched.h>
#endif
#include <iostream>
#ifdef Q_OS_FREEBSD
#include <libusb.h>
// FreeBSD doesn't have libusb_setlocale()
#define libusb_setlocale( x ) (void)0
#else
#include <libusb-1.0/libusb.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <math.h>
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

#ifdef Q_OS_WIN
    // close "extra" console window but if started from cmd.exe use this console
    if ( FreeConsole() && AttachConsole( ATTACH_PARENT_PROCESS ) ) {
        freopen( "CONOUT$", "w", stdout );
        freopen( "CONOUT$", "w", stderr );
    }
#endif

    bool demoMode = false;
    bool useGLES = false;
    bool useGLSL120 = false;
    bool useGLSL150 = false;
    bool useLocale = true;
    QString font = defaultFont;               // defined in viewsettings.h
    int fontSize = defaultFontSize;           // defined in viewsettings.h
    int condensed = defaultCondensed;         // defined in viewsettings.h
    QSettings *storeSettings = new QSettings; // delete later!
    storeSettings->beginGroup( "view" );
    if ( storeSettings->contains( "fontSize" ) )
        fontSize = Dso::InterpolationMode( storeSettings->value( "fontSize" ).toInt() );
    storeSettings->endGroup(); // view
    {
        QCoreApplication parserApp( argc, argv );
        QCommandLineParser p;
        p.addHelpOption();
        p.addVersionOption();
        QCommandLineOption demoModeOption( {"d", "demoMode"}, "Demo mode without scope HW" );
        p.addOption( demoModeOption );
        QCommandLineOption useGlesOption( {"e", "useGLES"}, "Use OpenGL ES instead of OpenGL" );
        p.addOption( useGlesOption );
        QCommandLineOption useGLSL120Option( "useGLSL120", "Force OpenGL SL version 1.20" );
        p.addOption( useGLSL120Option );
        QCommandLineOption useGLSL150Option( "useGLSL150", "Force OpenGL SL version 1.50" );
        p.addOption( useGLSL150Option );
        QCommandLineOption intOption( {"i", "international"}, "Show the international interface, do not translate" );
        p.addOption( intOption );
        QCommandLineOption fontOption( {"f", "font"}, "Define the system font", "Font" );
        p.addOption( fontOption );
        QCommandLineOption sizeOption(
            {"s", "size"}, QString( "Set the font size (default = %1, 0: automatic from dpi)" ).arg( fontSize ), "Size" );
        p.addOption( sizeOption );
        QCommandLineOption condensedOption(
            {"c", "condensed"}, QString( "Set the font condensed value (default = %1)" ).arg( condensed ), "Condensed" );
        p.addOption( condensedOption );
        p.process( parserApp );
        demoMode = p.isSet( demoModeOption );
        useGLES = p.isSet( useGlesOption );
        if ( p.isSet( fontOption ) )
            font = p.value( "font" );
        if ( p.isSet( sizeOption ) )
            fontSize = p.value( "size" ).toInt();
        if ( p.isSet( condensedOption ) ) // allow range from UltraCondensed (50) to UltraExpanded (200)
            condensed = qMin( qMax( p.value( "condensed" ).toInt(), 50 ), 200 );
        useGLSL120 = p.isSet( useGLSL120Option );
        useGLSL150 = p.isSet( useGLSL150Option );
        useLocale = !p.isSet( intOption );
    }

#ifdef Q_PROCESSOR_ARM
    // HACK: Raspberry Pi crashes with OpenGL, use always OpenGLES
    useGLES = true;
#endif

    GlScope::useQSurfaceFormat( useGLES ? QSurfaceFormat::OpenGLES : QSurfaceFormat::OpenGL );
    if ( useGLSL120 )
        GlScope::useOpenGLSLversion( 120 );
    else if ( useGLSL150 )
        GlScope::useOpenGLSLversion( 150 );
    QApplication openHantekApplication( argc, argv );

    // Qt5 linux default
    // ("Breeze", "Windows", "Fusion")
    // with package qt5-style-plugins
    // ("Breeze", "bb10dark", "bb10bright", "cleanlooks", "gtk2", "cde", "motif", "plastique", "Windows", "Fusion")
#ifndef Q_OS_MACOS
    openHantekApplication.setStyle( QStyleFactory::create( "Fusion" ) ); // smaller widgets allow stacking of all docks
#endif

    if ( 0 == fontSize ) { // calculate fontsize from screen dpi (96 dpi -> 10 point)
        fontSize = int( round( openHantekApplication.desktop()->logicalDpiY() / 9.6 ) );
        fontSize = qBound( 6, fontSize, 24 ); // values < 6 do not scale correctly
        // printf( "automatic fontSize: %d\n", fontSize );
    }
    QFont f = openHantekApplication.font();
    f.setFamily( font ); // Fusion style + Arial (default) -> fit on small screen (Y >= 720 pixel)
    f.setStretch( condensed );
    f.setPointSize( fontSize ); // scales the widgets accordingly
    openHantekApplication.setFont( f );
    openHantekApplication.setFont( f, "QWidget" ); // on some systems the 2nd argument is required
    storeSettings->beginGroup( "view" );
    storeSettings->setValue( "fontSize", fontSize );
    storeSettings->endGroup(); // view
    delete storeSettings;      // not needed anymore

#ifdef Q_OS_LINUX
    // try to set realtime priority to improve USB allocation
    // this works if the user is member of a realtime group, e.g. audio:
    // 1. set limits in /etc/security/limits.d:
    //    @audio - rtprio 99
    // 2. add user to the group, e.g. audio:
    //    usermod -a -G audio <your_user_name>
    // or set the limits only for your user in /etc/security/limits.d:
    //    <your_user_name> - rtprio 99
    struct sched_param schedParam;
    schedParam.sched_priority = 9;                    // set RT priority level 10
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
        if ( useLocale ) // localize USB error messages, supported: "en", "nl", "fr", "ru"
            libusb_setlocale( QLocale::system().name().toLocal8Bit().constData() );

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
    ExporterProcessor samplesToExportRaw( &exportRegistry );
    exportRegistry.registerExporter( &exporterCSV );

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
    std::cout << "OpenHantek6022 ";

    dsoControl.quitSampling(); // send USB control command, stop bulk transfer

    // stop the capturing thread
    unsigned waitForCapturing = unsigned( 2000 * dsoControl.getSamplesize() / dsoControl.getSamplerate() );
    if ( waitForCapturing < 10000 ) // minimum 10 s
        waitForCapturing = 10000;
    capturing.requestInterruption();
    capturing.wait( waitForCapturing );

    std::cout << "has ";

    // now quit the data acquisition thread
    // wait 2 * record time (delay is ms) for dso to finish
    unsigned waitForDso = unsigned( 2000 * dsoControl.getSamplesize() / dsoControl.getSamplerate() );
    if ( waitForDso < 10000 ) // minimum 10 s
        waitForDso = 10000;
    dsoControlThread.quit();
    dsoControlThread.wait( waitForDso );
    std::cout << "stopped ";

    // next stop the data processing
    postProcessing.stop();
    postProcessingThread.quit();
    postProcessingThread.wait( 10000 );
    std::cout << "after ";

    // finally shut down the libUSB communication
    if ( scopeDevice )
        scopeDevice.reset(); // destroys unique_pointer, causes libusb_close(), must be called before libusb_exit()
    if ( context )
        libusb_exit( context );
    std::cout << openHantekMainWindow.elapsedTime.elapsed() / 1000 << " s\n";

    return appStatus;
}
