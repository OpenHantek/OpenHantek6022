// SPDX-License-Identifier: GPL-2.0+

#include "mainwindow.h"
#include "iconfont/QtAwesome.h"
#include "ui_mainwindow.h"

#include "HorizontalDock.h"
#include "SpectrumDock.h"
#include "TriggerDock.h"
#include "VoltageDock.h"
#include "dockwindows.h"

#include "configdialog.h"
#include "dockwindows.h"
#include "dsomodel.h"
#include "dsowidget.h"
#include "exporting/exporterinterface.h"
#include "exporting/exporterregistry.h"
#include "hantekdsocontrol.h"
#include "usb/scopedevice.h"
#include "viewconstants.h"

#include "dsosettings.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPalette>
#include <QPrintDialog>
#include <QPrinter>

#include "OH_VERSION.h"

MainWindow::MainWindow( HantekDsoControl *dsoControl, DsoSettings *settings, ExporterRegistry *exporterRegistry, QWidget *parent )
    : QMainWindow( parent ), ui( new Ui::MainWindow ), dsoSettings( settings ), exporterRegistry( exporterRegistry ) {
    // suppress nasty warnings, e.g. "kf5.kio.core: Invalid URL ..." or "qt.qpa.xcb: QXcbConnection: XCB error: 3 (BadWindow) ..."
    QLoggingCategory::setFilterRules( "kf5.kio.core=false\nqt.qpa.xcb=false" );
    QVariantMap colorMap;
    QString iconPath = QString( ":/images/" );
    if ( QPalette().color( QPalette::Window ).lightness() < 128 ) { // automatic light/dark icon switch
        iconPath += "darktheme/";                                   // select top window icons accordingly
        colorMap.insert( "color-off", QColor( 208, 208, 208 ) );    // light grey normal
        colorMap.insert( "color-active", QColor( 255, 255, 255 ) ); // white when selected
    }
    elapsedTime.start();
    ui->setupUi( this );
    iconPause = QIcon( iconPath + "pause.svg" );
    iconPlay = QIcon( iconPath + "play.svg" );
    ui->actionSampling->setIcon( iconPause );
    QList< QKeySequence > shortcuts; // provide multiple shortcuts for ui->actionSampling.
    // 1st entry in list is shown as shortcut in menu
#ifdef Q_OS_WIN
    shortcuts << QKeySequence( Qt::Key::Key_S ); // WIN: <space> can be grabbed by buttons, e.g. CH1
#endif
    shortcuts << QKeySequence( Qt::Key::Key_Space );
    shortcuts << QKeySequence( Qt::Key::Key_Pause );
#ifndef Q_OS_WIN
    shortcuts << QKeySequence( Qt::Key::Key_S ); // else put this shortcut at the end of the list
#endif
    ui->actionSampling->setShortcuts( shortcuts );
    ui->actionRefresh->setIcon( QIcon( iconPath + "refresh.svg" ) );
    ui->actionRefresh->setShortcut( Qt::Key::Key_R );
    ui->actionPhosphor->setIcon( QIcon( iconPath + "phosphor.svg" ) );
    ui->actionPhosphor->setShortcut( Qt::Key::Key_P );
    ui->actionHistogram->setIcon( QIcon( iconPath + "histogram.svg" ) );
    ui->actionHistogram->setShortcut( Qt::Key::Key_H );
    ui->actionZoom->setIcon( QIcon( iconPath + "zoom.svg" ) );
    ui->actionZoom->setShortcut( Qt::Key::Key_Z );
    ui->actionMeasure->setIcon( QIcon( iconPath + "measure.svg" ) );
    ui->actionMeasure->setShortcut( Qt::Key::Key_M );
    ui->actionOpen->setIcon( iconFont->icon( fa::folderopen, colorMap ) );
    ui->actionSave->setIcon( iconFont->icon( fa::save, colorMap ) );
    ui->actionSettings->setIcon( iconFont->icon( fa::gear, colorMap ) );
    ui->actionManualCommand->setIcon( iconFont->icon( fa::edit, colorMap ) );
    ui->actionAbout->setIcon( iconFont->icon( fa::questioncircle, colorMap ) );
    ui->actionUserManual->setIcon( iconFont->icon( fa::filepdfo, colorMap ) );

    // Window title
    setWindowIcon( QIcon( ":/images/OpenHantek.svg" ) );
    setWindowTitle( dsoControl->getDevice()->isRealHW()
                        ? tr( "OpenHantek6022 (%1) - Device %2 (FW%3)" )
                              .arg( QString::fromStdString( VERSION ), QString::fromStdString( dsoControl->getModel()->name ) )
                              .arg( dsoControl->getDevice()->getFwVersion(), 4, 16, QChar( '0' ) )
                        : tr( "OpenHantek6022 (%1) - " ).arg( QString::fromStdString( VERSION ) ) + tr( "Demo Mode" ) );

#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 ) )
    setDockOptions( dockOptions() | QMainWindow::GroupedDragging );
#endif
    QAction *action;
    action = new QAction( iconFont->icon( fa::camera, colorMap ), tr( "Screenshot .." ), this );
    action->setToolTip( "Make a screenshot of the program window" );
    connect( action, &QAction::triggered, [this]() { screenShot( SCREENSHOT ); } );
    ui->menuExport->addAction( action );

    action = new QAction( iconFont->icon( fa::camera, colorMap ), tr( "Hardcopy .." ), this );
    action->setToolTip( "Make a (printable) hardcopy of the display" );
    connect( action, &QAction::triggered, [this]() {
        dsoWidget->switchToPrintColors();
        QTimer::singleShot( 20, [this]() { screenShot( HARDCOPY ); } );
    } );
    ui->menuExport->addAction( action );

    action = new QAction( iconFont->icon( fa::print, colorMap ), tr( "Print screen .." ), this );
    action->setToolTip( "Send the hardcopy to the printer" );
    connect( action, &QAction::triggered, [this]() {
        dsoWidget->switchToPrintColors();
        QTimer::singleShot( 20, [this]() { screenShot( PRINTER ); } );
    } );
    ui->menuExport->addAction( action );

    ui->menuExport->addSeparator();

    for ( auto *exporter : *exporterRegistry ) {
        action = new QAction( iconFont->icon( exporter->faIcon(), colorMap ), exporter->name(), this );
        action->setCheckable( exporter->type() == ExporterInterface::Type::ContinousExport );
        connect( action, &QAction::triggered, [exporter, exporterRegistry]( bool checked ) {
            exporterRegistry->setExporterEnabled( exporter,
                                                  exporter->type() == ExporterInterface::Type::ContinousExport ? checked : true );
        } );
        ui->menuExport->addAction( action );
    }

    DsoSettingsScope *scope = &( dsoSettings->scope );
    const Dso::ControlSpecification *spec = dsoControl->getModel()->spec();

    registerDockMetaTypes();

    // Docking windows
    // Create dock windows before the dso widget, they fix messed up settings

    VoltageDock *voltageDock = new VoltageDock( scope, spec, this );
    HorizontalDock *horizontalDock = new HorizontalDock( scope, spec, this );
    TriggerDock *triggerDock = new TriggerDock( scope, spec, this );
    SpectrumDock *spectrumDock = new SpectrumDock( scope, this );

    addDockWidget( Qt::RightDockWidgetArea, voltageDock );
    addDockWidget( Qt::RightDockWidgetArea, horizontalDock );
    addDockWidget( Qt::RightDockWidgetArea, triggerDock );
    addDockWidget( Qt::RightDockWidgetArea, spectrumDock );

    restoreGeometry( dsoSettings->mainWindowGeometry );
    restoreState( dsoSettings->mainWindowState );

    // Central oszilloscope widget
    dsoWidget = new DsoWidget( &dsoSettings->scope, &dsoSettings->view, spec );
    setCentralWidget( dsoWidget );

    // Command field inside the status bar
    commandEdit = new QLineEdit( this );
    commandEdit->hide();

    statusBar()->addPermanentWidget( commandEdit, 1 );

    connect( ui->actionManualCommand, &QAction::toggled, [this]( bool checked ) {
        commandEdit->setVisible( checked );
        if ( checked )
            commandEdit->setFocus();
    } );

    connect( commandEdit, &QLineEdit::returnPressed, [this, dsoControl]() {
        Dso::ErrorCode errorCode = dsoControl->stringCommand( commandEdit->text() );
        commandEdit->clear();
        this->ui->actionManualCommand->setChecked( false );
        if ( errorCode != Dso::ErrorCode::NONE )
            statusBar()->showMessage( tr( "Invalid command" ), 3000 );
    } );

    // Connect general signals
    connect( dsoControl, &HantekDsoControl::statusMessage, statusBar(), &QStatusBar::showMessage );

    // Connect signals to DSO controller and widget
    connect( horizontalDock, &HorizontalDock::samplerateChanged, [dsoControl, this]() {
        dsoControl->setSamplerate( dsoSettings->scope.horizontal.samplerate );
        this->dsoWidget->updateSamplerate( dsoSettings->scope.horizontal.samplerate );
    } );
    connect( horizontalDock, &HorizontalDock::timebaseChanged, [dsoControl, this]() {
        dsoControl->setRecordTime( dsoSettings->scope.horizontal.timebase * DIVS_TIME );
        this->dsoWidget->updateTimebase( dsoSettings->scope.horizontal.timebase );
    } );
    connect( horizontalDock, &HorizontalDock::frequencybaseChanged, dsoWidget, &DsoWidget::updateFrequencybase );
    connect( dsoControl, &HantekDsoControl::samplerateChanged, [this, horizontalDock]( double samplerate ) {
        // The timebase was set, let's adapt the samplerate accordingly
        // printf( "mainwindow::samplerateChanged( %g )\n", samplerate );
        dsoSettings->scope.horizontal.samplerate = samplerate;
        horizontalDock->setSamplerate( samplerate );
        dsoWidget->updateSamplerate( samplerate );
    } );
    connect( horizontalDock, &HorizontalDock::calfreqChanged,
             [dsoControl, this]() { dsoControl->setCalFreq( dsoSettings->scope.horizontal.calfreq ); } );
    connect( horizontalDock, &HorizontalDock::formatChanged,
             [this]( Dso::GraphFormat format ) { ui->actionHistogram->setEnabled( format == Dso::GraphFormat::TY ); } );

    connect( triggerDock, &TriggerDock::modeChanged, dsoControl, &HantekDsoControl::setTriggerMode );
    connect( triggerDock, &TriggerDock::modeChanged, dsoWidget, &DsoWidget::updateTriggerMode );
    connect( triggerDock, &TriggerDock::modeChanged,
             [this]( Dso::TriggerMode mode ) { ui->actionRefresh->setEnabled( Dso::TriggerMode::ROLL == mode ); } );
    connect( triggerDock, &TriggerDock::sourceChanged, dsoControl, &HantekDsoControl::setTriggerSource );
    connect( triggerDock, &TriggerDock::sourceChanged, dsoWidget, &DsoWidget::updateTriggerSource );
    connect( triggerDock, &TriggerDock::slopeChanged, dsoControl, &HantekDsoControl::setTriggerSlope );
    connect( triggerDock, &TriggerDock::slopeChanged, dsoWidget, &DsoWidget::updateTriggerSlope );
    connect( dsoWidget, &DsoWidget::triggerPositionChanged, dsoControl, &HantekDsoControl::setTriggerOffset );
    connect( dsoWidget, &DsoWidget::triggerLevelChanged, dsoControl, &HantekDsoControl::setTriggerLevel );

    auto usedChanged = [this, dsoControl, spec]( ChannelID channel ) {
        if ( channel >= dsoSettings->scope.voltage.size() )
            return;

        bool mathUsed = dsoSettings->scope.anyUsed( spec->channels );

        // Normal channel, check if voltage/spectrum or math channel is used
        if ( channel < spec->channels )
            dsoControl->setChannelUsed( channel, mathUsed | dsoSettings->scope.anyUsed( channel ) );
        // Math channel, update all channels
        else if ( channel == spec->channels ) {
            for ( ChannelID c = 0; c < spec->channels; ++c )
                dsoControl->setChannelUsed( c, mathUsed | dsoSettings->scope.anyUsed( c ) );
        }
    };
    connect( voltageDock, &VoltageDock::usedChanged, usedChanged );
    connect( spectrumDock, &SpectrumDock::usedChanged, usedChanged );

    connect( voltageDock, &VoltageDock::modeChanged, dsoWidget, &DsoWidget::updateMathMode );
    connect( voltageDock, &VoltageDock::gainChanged, [dsoControl, spec]( ChannelID channel, double gain ) {
        if ( channel >= spec->channels )
            return;
        dsoControl->setGain( channel, gain );
    } );
    connect( voltageDock, &VoltageDock::probeAttnChanged, [dsoControl, spec]( ChannelID channel, double probeAttn ) {
        if ( channel >= spec->channels )
            return;
        dsoControl->setProbe( channel, probeAttn );
    } );
    connect( voltageDock, &VoltageDock::invertedChanged, [dsoControl, spec]( ChannelID channel, bool inverted ) {
        if ( channel >= spec->channels )
            return;
        dsoControl->setChannelInverted( channel, inverted );
    } );
    connect( voltageDock, &VoltageDock::couplingChanged, dsoWidget, &DsoWidget::updateVoltageCoupling );
    connect( voltageDock, &VoltageDock::couplingChanged, [dsoControl, spec]( ChannelID channel, Dso::Coupling coupling ) {
        if ( channel >= spec->channels )
            return;
        dsoControl->setCoupling( channel, coupling );
    } );
    connect( voltageDock, &VoltageDock::gainChanged, dsoWidget, &DsoWidget::updateVoltageGain );
    connect( voltageDock, &VoltageDock::usedChanged, dsoWidget, &DsoWidget::updateVoltageUsed );
    connect( spectrumDock, &SpectrumDock::usedChanged, dsoWidget, &DsoWidget::updateSpectrumUsed );
    connect( spectrumDock, &SpectrumDock::magnitudeChanged, dsoWidget, &DsoWidget::updateSpectrumMagnitude );

    // Started/stopped signals from oscilloscope
    connect( dsoControl, &HantekDsoControl::samplingStatusChanged, [this]( bool enabled ) {
        QSignalBlocker blocker( this->ui->actionSampling );
        if ( enabled ) {
            this->ui->actionSampling->setIcon( this->iconPause );
            this->ui->actionSampling->setText( tr( "Stop" ) );
            this->ui->actionSampling->setStatusTip( tr( "Stop the oscilloscope" ) );
        } else {
            this->ui->actionSampling->setIcon( this->iconPlay );
            this->ui->actionSampling->setText( tr( "Start" ) );
            this->ui->actionSampling->setStatusTip( tr( "Start the oscilloscope" ) );
        }
        this->ui->actionSampling->setChecked( enabled );
    } );
    connect( this->ui->actionSampling, &QAction::triggered, dsoControl, &HantekDsoControl::enableSampling );
    this->ui->actionSampling->setChecked( dsoControl->isSampling() );

    connect( this->ui->actionRefresh, &QAction::triggered, dsoControl, &HantekDsoControl::restartSampling );

    connect( dsoControl, &HantekDsoControl::samplerateLimitsChanged, horizontalDock, &HorizontalDock::setSamplerateLimits );
    connect( dsoControl, &HantekDsoControl::samplerateSet, horizontalDock, &HorizontalDock::setSamplerateSteps );

    // Load settings to GUI
    connect( this, &MainWindow::settingsLoaded, voltageDock, &VoltageDock::loadSettings );
    connect( this, &MainWindow::settingsLoaded, horizontalDock, &HorizontalDock::loadSettings );
    connect( this, &MainWindow::settingsLoaded, spectrumDock, &SpectrumDock::loadSettings );
    connect( this, &MainWindow::settingsLoaded, triggerDock, &TriggerDock::loadSettings );
    connect( this, &MainWindow::settingsLoaded, dsoControl, &HantekDsoControl::applySettings );
    connect( this, &MainWindow::settingsLoaded, dsoWidget, &DsoWidget::updateSlidersSettings );

    connect( ui->actionOpen, &QAction::triggered, [this, spec]() {
        QString fileName = QFileDialog::getOpenFileName( this, tr( "Open file" ), "", tr( "Settings (*.conf)" ) );
        if ( !fileName.isEmpty() ) {
            if ( dsoSettings->setFilename( fileName ) ) {
                dsoSettings->load();
            }
            emit settingsLoaded( &dsoSettings->scope, spec );

            dsoWidget->updateTimebase( dsoSettings->scope.horizontal.timebase );

            for ( ChannelID channel = 0; channel < spec->channels; ++channel ) {
                this->dsoWidget->updateVoltageUsed( channel, dsoSettings->scope.voltage[ channel ].used );
                this->dsoWidget->updateSpectrumUsed( channel, dsoSettings->scope.spectrum[ channel ].used );
            }
        }
    } );

    connect( ui->actionSave, &QAction::triggered, [this]() {
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();
        dsoSettings->save();
    } );

    connect( ui->actionSave_as, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getSaveFileName( this, tr( "Save settings" ), "", tr( "Settings (*.conf)" ) );
        if ( fileName.isEmpty() )
            return;
        if ( !fileName.endsWith( ".conf" ) )
            fileName.append( ".conf" );
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();
        dsoSettings->setFilename( fileName );
        dsoSettings->save();
    } );

    connect( ui->actionExit, &QAction::triggered, this, &QWidget::close );

    connect( ui->actionSettings, &QAction::triggered, [this]() {
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();

        DsoConfigDialog *configDialog = new DsoConfigDialog( this->dsoSettings, this );
        configDialog->setModal( true );
        configDialog->show();
    } );

    connect( this->ui->actionPhosphor, &QAction::toggled, [this]( bool enabled ) {
        dsoSettings->view.digitalPhosphor = enabled;

        if ( dsoSettings->view.digitalPhosphor )
            this->ui->actionPhosphor->setStatusTip( tr( "Disable fading of previous graphs" ) );
        else
            this->ui->actionPhosphor->setStatusTip( tr( "Enable fading of previous graphs" ) );
    } );
    this->ui->actionPhosphor->setChecked( dsoSettings->view.digitalPhosphor );

    connect( ui->actionHistogram, &QAction::toggled, [this]( bool enabled ) {
        dsoSettings->scope.histogram = enabled;

        if ( dsoSettings->scope.histogram )
            this->ui->actionHistogram->setStatusTip( tr( "Hide histogram" ) );
        else
            this->ui->actionHistogram->setStatusTip( tr( "Show histogram" ) );
    } );
    ui->actionHistogram->setChecked( dsoSettings->scope.histogram );
    ui->actionHistogram->setEnabled( scope->horizontal.format == Dso::GraphFormat::TY );

    connect( ui->actionZoom, &QAction::toggled, [this]( bool enabled ) {
        dsoSettings->view.zoom = enabled;

        if ( dsoSettings->view.zoom )
            this->ui->actionZoom->setStatusTip( tr( "Hide magnified scope" ) );
        else
            this->ui->actionZoom->setStatusTip( tr( "Show magnified scope" ) );

        this->dsoWidget->updateZoom( enabled );
    } );
    ui->actionZoom->setChecked( dsoSettings->view.zoom );

    connect( ui->actionMeasure, &QAction::toggled, [this]( bool enabled ) {
        dsoSettings->view.cursorsVisible = enabled;

        if ( dsoSettings->view.cursorsVisible )
            this->ui->actionMeasure->setStatusTip( tr( "Hide measurements" ) );
        else
            this->ui->actionMeasure->setStatusTip( tr( "Show measurements" ) );

        this->dsoWidget->updateCursorGrid( enabled );
    } );
    ui->actionMeasure->setChecked( dsoSettings->view.cursorsVisible );

    connect( ui->actionUserManual, &QAction::triggered, []() {
        QString usrManualPath( USR_MANUAL_PATH );
        QFile userManual( usrManualPath );
        if ( userManual.exists() )
            QDesktopServices::openUrl( QUrl( "file://" + usrManualPath ) );
        else
            QDesktopServices::openUrl(
                QUrl( "https://github.com/OpenHantek/OpenHantek6022/blob/master/docs/OpenHantek6022_User_Manual.pdf" ) );
    } );

    connect( ui->actionAbout, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr( "About OpenHantek6022 (%1)" ).arg( VERSION ),
            tr( "<p>Open source software for Hantek6022 USB oscilloscopes</p>"
                "<p>Maintainer: Martin Homuth-Rosemann</p>"
                "<p>Copyright &copy; 2010, 2011 Oliver Haag</p>"
                "<p>Copyright &copy; 2012-2020 OpenHantek community<br/>"
                "<a href='https://github.com/OpenHantek'>https://github.com/OpenHantek</a></p>"
                "<p>Open source firmware copyright &copy; 2019-2020 Ho-Ro<br/>"
                "<a href='https://github.com/Ho-Ro/Hantek6022API'>https://github.com/Ho-Ro/Hantek6022API</a></p>" ) +
                tr( "<p>Running since %1 seconds.</p>" ).arg( elapsedTime.elapsed() / 1000 ) );
    } );

    emit settingsLoaded( &dsoSettings->scope, spec );

    dsoWidget->updateTimebase( dsoSettings->scope.horizontal.timebase );

    for ( ChannelID channel = 0; channel < spec->channels; ++channel ) {
        this->dsoWidget->updateVoltageUsed( channel, dsoSettings->scope.voltage[ channel ].used );
        this->dsoWidget->updateSpectrumUsed( channel, dsoSettings->scope.spectrum[ channel ].used );
    }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::showNewData( std::shared_ptr< PPresult > newData ) { dsoWidget->showNew( newData ); }

void MainWindow::exporterStatusChanged( const QString &exporterName, const QString &status ) {
    ui->statusbar->showMessage( tr( "%1: %2" ).arg( exporterName, status ) );
}

void MainWindow::exporterProgressChanged() { exporterRegistry->checkForWaitingExporters(); }

// make screenshot (type == SCREENSHOT) from the complete program window with screen colors ...
// ... or a printable hardcopy (type == HARDCOPY) from the scope widget only ...
// ... with printer colors and scaled to double height for zoomed screens ...
// ... or send this hardcopy directly to the printer (type == PRINTER).
void MainWindow::screenShot( screenshotType_t screenshotType ) {
    auto activeWindow = screenshotType == SCREENSHOT ? qApp->activeWindow() : dsoWidget;
    QPixmap screenshot( activeWindow->size() );
    QDateTime now = QDateTime::currentDateTime();
    QString docName = now.toString( tr( "yyyy-MM-dd hh:mm:ss" ) );
    QString fileName = now.toString( tr( "yyyyMMdd_hhmmss" ) );
    commandEdit->setText( docName ); // show date in bottom line
    commandEdit->setVisible( true );
    activeWindow->render( &screenshot ); // take the screenshot
    commandEdit->clear();
    commandEdit->setVisible( false );
    dsoWidget->restoreScreenColors();

    int sw = screenshot.width();
    int sh = screenshot.height();
    if ( screenshotType != SCREENSHOT && dsoSettings->view.zoom && dsoSettings->view.zoomImage ) {
        screenshot = screenshot.scaled( sw, sh *= 2 ); // make double height
    }

    // here we have a screeshot, now handle the different destinations.
    QPrinter printer( QPrinter::HighResolution );
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 10, 0 ) )
    printer.setPdfVersion( QPrinter::PdfVersion_A1b );
#endif
    printer.setPaperSize( QPrinter::A4 );
    printer.setPageMargins( 20, 20, 20, 20, QPrinter::Millimeter );
    printer.setOrientation( sw > sh ? QPrinter::Landscape : QPrinter::Portrait );
    printer.setCreator( QCoreApplication::applicationName() );
    printer.setDocName( docName );

    if ( screenshotType != PRINTER ) { // ask for a filename
        QStringList filters;
        fileName += ".png";
        filters << tr( "Image (*.png *.jpg)" ) << tr( "Portable Document Format (*.pdf)" );
        QFileDialog fileDialog( this, tr( "Save screenshot" ), fileName, filters.join( ";;" ) );
        fileDialog.setAcceptMode( QFileDialog::AcceptSave );
        if ( fileDialog.exec() != QDialog::Accepted )
            return;

        fileName = fileDialog.selectedFiles().first();
        if ( filters.indexOf( fileDialog.selectedNameFilter() ) == 0 ) { // save as image
            screenshot.save( fileName );
            return;
        }

        // else create a *.pdf with a scaled and centered image
        printer.setOutputFormat( QPrinter::PdfFormat );
        printer.setOutputFileName( fileName );
        // supports screen resolution up to about 9600 x 9600 pixel
        int resolution = 75;
        printer.setResolution( resolution );
        int pw = printer.pageRect().width();
        int ph = printer.pageRect().height();
        int scale = qMin( pw / sw, ph / sh );
        while ( scale < 2 && resolution < 1200 ) {
            resolution *= 2;
            printer.setResolution( resolution );
            pw = printer.pageRect().width();
            ph = printer.pageRect().height();
            scale = qMin( pw / sw, ph / sh );
        }
    } else { // Show the printing dialog
        printer.setDocName( fileName + ".pdf" );
        QPrintDialog dialog( &printer );
        dialog.setWindowTitle( tr( "Print oscillograph" ) );
        if ( dialog.exec() != QDialog::Accepted ) {
            return;
        }
    }
    // send the pixmap to *.pdf or printer
    int pw = printer.pageRect().width();
    int ph = printer.pageRect().height();
    int scale = qMin( pw / sw, ph / sh );

    // qDebug() << sw << sh << pw << ph << scale << resolution;
    if ( scale < 1 )
        qDebug() << "Screenshot size too big, page will be cropped";
    else if ( scale > 1 ) // upscale accordingly
        screenshot = screenshot.scaled( sw *= scale, sh *= scale );
    printer.newPage();
    QPainter p( &printer );
    p.drawPixmap( ( pw - sw ) / 2, ( ph - sh ) / 2, screenshot ); // center the picture
    p.end();
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void MainWindow::closeEvent( QCloseEvent *event ) {
    if ( dsoSettings->alwaysSave ) {
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();
        dsoSettings->save();
    }
    QMainWindow::closeEvent( event );
}
