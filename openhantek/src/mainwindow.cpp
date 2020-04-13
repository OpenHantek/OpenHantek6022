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
#include "usb/usbdevice.h"
#include "viewconstants.h"

#include "dsosettings.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QDesktopServices>
#include <QPalette>

#include "OH_VERSION.h"

MainWindow::MainWindow(HantekDsoControl *dsoControl, DsoSettings *settings, ExporterRegistry *exporterRegistry,
                       QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), dsoSettings(settings), exporterRegistry(exporterRegistry) {

    QVariantMap colorMap;
    QString iconPath = QString( ":/images/" );
    if ( QPalette().color( QPalette::Window ).lightness() < 128 ) {// automatic light/dark icon switch
        iconPath += "darktheme/"; // select top window icons accordingly
        colorMap.insert( "color-off", QColor( 208, 208, 208 ) ); // light grey normal
        colorMap.insert( "color-active", QColor( 255, 255, 255 ) ); // white when selected
    }

    ui->setupUi(this);
    iconPause = QIcon( iconPath + "pause.svg" );
    iconPlay = QIcon( iconPath + "play.svg" );
    ui->actionSampling->setIcon( iconPause );
    ui->actionDigital_phosphor->setIcon( QIcon( iconPath + "digitalphosphor.svg" ) );
    ui->actionHistogram->setIcon( QIcon( iconPath + "histogram.svg" ) );
    ui->actionZoom->setIcon( QIcon( iconPath + "zoom.svg" ) );
    ui->actionMeasure->setIcon( QIcon( iconPath + "measure.svg" ) );
    ui->actionOpen->setIcon(iconFont->icon( fa::folderopen, colorMap ) );
    ui->actionSave->setIcon(iconFont->icon( fa::save, colorMap ) );
    ui->actionSettings->setIcon(iconFont->icon( fa::gear, colorMap ) );
    ui->actionManualCommand->setIcon(iconFont->icon( fa::edit, colorMap ) );
    ui->actionAbout->setIcon(iconFont->icon( fa::questioncircle, colorMap ) );
    ui->actionUserManual->setIcon(iconFont->icon( fa::filepdfo, colorMap ) );

    // Window title
    setWindowIcon( QIcon( ":/images/OpenHantek.svg" ) );
    setWindowTitle(
        tr("OpenHantek6022 (%1) - Device %2 (FW%3)")
            .arg( QString::fromStdString( VERSION),
                  QString::fromStdString(dsoControl->getDevice()->getModel()->name))
            .arg(dsoControl->getDevice()->getFwVersion(),4,16,QChar('0'))
    );

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif

    for (auto *exporter : *exporterRegistry) {
        QAction *action = new QAction(iconFont->icon( exporter->faIcon(), colorMap ), exporter->name(), this);
        action->setCheckable(exporter->type() == ExporterInterface::Type::ContinousExport);
        connect(action, &QAction::triggered, [exporter, exporterRegistry](bool checked) {
            exporterRegistry->setExporterEnabled(
                exporter, exporter->type() == ExporterInterface::Type::ContinousExport ? checked : true);
        });
        ui->menuExport->addAction(action);
    }

    DsoSettingsScope *scope = &(dsoSettings->scope);
    const Dso::ControlSpecification *spec = dsoControl->getDevice()->getModel()->spec();

    registerDockMetaTypes();

    // Docking windows
    // Create dock windows before the dso widget, they fix messed up settings
    VoltageDock *voltageDock;
    HorizontalDock *horizontalDock;
    TriggerDock *triggerDock;
    SpectrumDock *spectrumDock;
    voltageDock = new VoltageDock(scope, spec, this);
    horizontalDock = new HorizontalDock(scope,spec, this);
    triggerDock = new TriggerDock(scope, spec, this);
    spectrumDock = new SpectrumDock(scope, this);

    addDockWidget(Qt::RightDockWidgetArea, voltageDock);
    addDockWidget(Qt::RightDockWidgetArea, horizontalDock);
    addDockWidget(Qt::RightDockWidgetArea, triggerDock);
    addDockWidget(Qt::RightDockWidgetArea, spectrumDock);

    restoreGeometry(dsoSettings->mainWindowGeometry);
    restoreState(dsoSettings->mainWindowState);

    // Central oszilloscope widget
    dsoWidget = new DsoWidget(&dsoSettings->scope, &dsoSettings->view, spec);
    setCentralWidget(dsoWidget);

    // Command field inside the status bar
    QLineEdit *commandEdit = new QLineEdit(this);
    commandEdit->hide();

    statusBar()->addPermanentWidget(commandEdit, 1);

    connect(ui->actionManualCommand, &QAction::toggled, [ commandEdit](bool checked) {
        commandEdit->setVisible(checked);
        if (checked)
            commandEdit->setFocus();
    });

    connect(commandEdit, &QLineEdit::returnPressed, [this, commandEdit, dsoControl]() {
        Dso::ErrorCode errorCode = dsoControl->stringCommand(commandEdit->text());
        commandEdit->clear();
        this->ui->actionManualCommand->setChecked(false);
        if (errorCode != Dso::ErrorCode::NONE)
            statusBar()->showMessage(tr("Invalid command"), 3000);
    });

    // Connect general signals
    connect(dsoControl, &HantekDsoControl::statusMessage, statusBar(), &QStatusBar::showMessage);

    // Connect signals to DSO controller and widget
    connect(horizontalDock, &HorizontalDock::samplerateChanged, [dsoControl, this]() {
        dsoControl->setSamplerate(dsoSettings->scope.horizontal.samplerate);
        this->dsoWidget->updateSamplerate(dsoSettings->scope.horizontal.samplerate);
    });
    connect(horizontalDock, &HorizontalDock::timebaseChanged, [dsoControl, this]() {
        dsoControl->setRecordTime(dsoSettings->scope.horizontal.timebase * DIVS_TIME);
        this->dsoWidget->updateTimebase(dsoSettings->scope.horizontal.timebase);
    });
    connect(horizontalDock, &HorizontalDock::frequencybaseChanged, dsoWidget, &DsoWidget::updateFrequencybase);
    connect(dsoControl, &HantekDsoControl::samplerateChanged, [this, horizontalDock](double samplerate) {
        // The timebase was set, let's adapt the samplerate accordingly
        //printf( "mainwindow::samplerateChanged( %g )\n", samplerate );
        dsoSettings->scope.horizontal.samplerate = samplerate;
        horizontalDock->setSamplerate(samplerate);
        dsoWidget->updateSamplerate(samplerate);
    });
    connect(horizontalDock, &HorizontalDock::calfreqChanged, [dsoControl, this]() {
        dsoControl->setCalFreq(dsoSettings->scope.horizontal.calfreq);
    });
    connect(horizontalDock, &HorizontalDock::formatChanged, [this]( Dso::GraphFormat format ) {
        ui->actionHistogram->setEnabled( format == Dso::GraphFormat::TY );
    });

    connect(triggerDock, &TriggerDock::modeChanged, dsoControl, &HantekDsoControl::setTriggerMode);
    connect(triggerDock, &TriggerDock::modeChanged, dsoWidget, &DsoWidget::updateTriggerMode);
    connect(triggerDock, &TriggerDock::sourceChanged, dsoControl, &HantekDsoControl::setTriggerSource);
    connect(triggerDock, &TriggerDock::sourceChanged, dsoWidget, &DsoWidget::updateTriggerSource);
    connect(triggerDock, &TriggerDock::slopeChanged, dsoControl, &HantekDsoControl::setTriggerSlope);
    connect(triggerDock, &TriggerDock::slopeChanged, dsoWidget, &DsoWidget::updateTriggerSlope);
    connect(dsoWidget, &DsoWidget::triggerPositionChanged, dsoControl, &HantekDsoControl::setTriggerOffset );
    connect(dsoWidget, &DsoWidget::triggerLevelChanged, dsoControl, &HantekDsoControl::setTriggerLevel);

    auto usedChanged = [this, dsoControl, spec](ChannelID channel) {
        if (channel >= dsoSettings->scope.voltage.size())
            return;

        bool mathUsed = dsoSettings->scope.anyUsed(spec->channels);

        // Normal channel, check if voltage/spectrum or math channel is used
        if (channel < spec->channels)
            dsoControl->setChannelUsed(channel, mathUsed | dsoSettings->scope.anyUsed(channel));
        // Math channel, update all channels
        else if (channel == spec->channels) {
            for (ChannelID c = 0; c < spec->channels; ++c)
                dsoControl->setChannelUsed(c, mathUsed | dsoSettings->scope.anyUsed(c));
        }
    };
    connect(voltageDock, &VoltageDock::usedChanged, usedChanged);
    connect(spectrumDock, &SpectrumDock::usedChanged, usedChanged);

    connect(voltageDock, &VoltageDock::modeChanged, dsoWidget, &DsoWidget::updateMathMode);
    connect(voltageDock, &VoltageDock::gainChanged, [this, dsoControl, spec](ChannelID channel ) {
        if (channel >= spec->channels)
            return;
        dsoControl->setGain(channel, dsoSettings->scope.gain(channel) * DIVS_VOLTAGE);
    });
    connect(voltageDock, &VoltageDock::probeAttnChanged, [ dsoControl, spec](ChannelID channel, double probeAttn ) {
        if (channel >= spec->channels)
            return;
        dsoControl->setProbe( channel, probeAttn );
    });
    connect(voltageDock, &VoltageDock::invertedChanged, [ dsoControl, spec](ChannelID channel, bool inverted) {
        if (channel >= spec->channels)
            return;
        dsoControl->setChannelInverted( channel, inverted );
    });
    connect(voltageDock, &VoltageDock::couplingChanged, dsoWidget, &DsoWidget::updateVoltageCoupling);
    connect(voltageDock, &VoltageDock::couplingChanged, [ dsoControl, spec](ChannelID channel, Dso::Coupling coupling ) {
        if (channel >= spec->channels)
            return;
        dsoControl->setCoupling( channel, coupling );
    });
    connect(voltageDock, &VoltageDock::gainChanged, dsoWidget, &DsoWidget::updateVoltageGain);
    connect(voltageDock, &VoltageDock::usedChanged, dsoWidget, &DsoWidget::updateVoltageUsed);
    connect(spectrumDock, &SpectrumDock::usedChanged, dsoWidget, &DsoWidget::updateSpectrumUsed);
    connect(spectrumDock, &SpectrumDock::magnitudeChanged, dsoWidget, &DsoWidget::updateSpectrumMagnitude);

    // Started/stopped signals from oscilloscope
    connect(dsoControl, &HantekDsoControl::samplingStatusChanged, [this](bool enabled) {
        QSignalBlocker blocker(this->ui->actionSampling);
        if (enabled) {
            this->ui->actionSampling->setIcon( this->iconPause );
            this->ui->actionSampling->setText(tr("Stop"));
            this->ui->actionSampling->setStatusTip(tr("Stop the oscilloscope"));
        } else {
            this->ui->actionSampling->setIcon( this->iconPlay );
            this->ui->actionSampling->setText(tr("Start"));
            this->ui->actionSampling->setStatusTip(tr("Start the oscilloscope"));
        }
        this->ui->actionSampling->setChecked(enabled);
    });
    connect(this->ui->actionSampling, &QAction::triggered, dsoControl, &HantekDsoControl::enableSampling);
    this->ui->actionSampling->setChecked(dsoControl->isSampling());

    connect(dsoControl, &HantekDsoControl::samplerateLimitsChanged, horizontalDock,
            &HorizontalDock::setSamplerateLimits);
    connect(dsoControl, &HantekDsoControl::samplerateSet, horizontalDock, &HorizontalDock::setSamplerateSteps);

    // Load settings to GUI
    connect(this, &MainWindow::settingsLoaded, voltageDock, &VoltageDock::loadSettings);
    connect(this, &MainWindow::settingsLoaded, horizontalDock, &HorizontalDock::loadSettings);
    connect(this, &MainWindow::settingsLoaded, spectrumDock, &SpectrumDock::loadSettings);
    connect(this, &MainWindow::settingsLoaded, triggerDock, &TriggerDock::loadSettings);
    connect(this, &MainWindow::settingsLoaded, dsoControl, &HantekDsoControl::applySettings);
    connect(this, &MainWindow::settingsLoaded, dsoWidget, &DsoWidget::updateSlidersSettings);

    connect(ui->actionOpen, &QAction::triggered, [this, spec]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr("Settings (*.ini)"));
        if (!fileName.isEmpty()) {
            if (dsoSettings->setFilename(fileName)) { dsoSettings->load(); }
            emit settingsLoaded(&dsoSettings->scope, spec);

            dsoWidget->updateTimebase(dsoSettings->scope.horizontal.timebase);

            for (ChannelID channel = 0; channel < spec->channels; ++channel) {
                this->dsoWidget->updateVoltageUsed(channel, dsoSettings->scope.voltage[channel].used);
                this->dsoWidget->updateSpectrumUsed(channel, dsoSettings->scope.spectrum[channel].used);
            }
        }
    });

    connect(ui->actionSave, &QAction::triggered, [this]() {
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();
        dsoSettings->save();
    });

    connect(ui->actionSave_as, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings"), "", tr("Settings (*.ini)"));
        if (fileName.isEmpty())
            return;
        if (!fileName.endsWith(".ini"))
            fileName.append(".ini");
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();
        dsoSettings->setFilename(fileName);
        dsoSettings->save();
    });

    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionSettings, &QAction::triggered, [this]() {
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();

        DsoConfigDialog *configDialog = new DsoConfigDialog(this->dsoSettings, this);
        configDialog->setModal(true);
        configDialog->show();
    });

    connect(this->ui->actionDigital_phosphor, &QAction::toggled, [this](bool enabled) {
        dsoSettings->view.digitalPhosphor = enabled;

        if (dsoSettings->view.digitalPhosphor)
            this->ui->actionDigital_phosphor->setStatusTip(tr("Disable fading of previous graphs"));
        else
            this->ui->actionDigital_phosphor->setStatusTip(tr("Enable fading of previous graphs"));
    });
    this->ui->actionDigital_phosphor->setChecked(dsoSettings->view.digitalPhosphor);

    connect(ui->actionHistogram, &QAction::toggled, [this](bool enabled) {
        dsoSettings->scope.histogram = enabled;

        if (dsoSettings->scope.histogram)
            this->ui->actionHistogram->setStatusTip(tr("Hide histogram"));
        else
            this->ui->actionHistogram->setStatusTip(tr("Show histogram"));
    });
    ui->actionHistogram->setChecked(dsoSettings->scope.histogram);
    ui->actionHistogram->setEnabled( scope->horizontal.format == Dso::GraphFormat::TY );

    connect(ui->actionZoom, &QAction::toggled, [this](bool enabled) {
        dsoSettings->view.zoom = enabled;

        if (dsoSettings->view.zoom)
            this->ui->actionZoom->setStatusTip(tr("Hide magnified scope"));
        else
            this->ui->actionZoom->setStatusTip(tr("Show magnified scope"));

        this->dsoWidget->updateZoom(enabled);
    });
    ui->actionZoom->setChecked(dsoSettings->view.zoom);

    connect(ui->actionMeasure, &QAction::toggled, [this](bool enabled) {
        dsoSettings->view.cursorsVisible = enabled;

        if (dsoSettings->view.cursorsVisible)
            this->ui->actionMeasure->setStatusTip(tr("Hide measurements"));
        else
            this->ui->actionMeasure->setStatusTip(tr("Show measurements"));

        this->dsoWidget->updateCursorGrid(enabled);
    });
    ui->actionMeasure->setChecked(dsoSettings->view.cursorsVisible);

    connect(ui->actionUserManual, &QAction::triggered, []() {
        QString usrManualPath( USR_MANUAL_PATH );
        QFile userManual( usrManualPath );
        if ( userManual.exists() )
            QDesktopServices::openUrl( QUrl( "file://" + usrManualPath ) );
        else
            QDesktopServices::openUrl( QUrl( "https://github.com/OpenHantek/OpenHantek6022/blob/master/docs/OpenHantek6022_User_Manual.pdf" ) );
    });

    connect(ui->actionAbout, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr("About OpenHantek6022 (%1)").arg(VERSION),
            tr("<p>Open source software for Hantek6022 USB oscilloscopes</p>"
               "<p>Copyright &copy; 2010, 2011 Oliver Haag</p>"
               "<p>Copyright &copy; 2012-2020 OpenHantek community<br/>"
               "<a href='https://github.com/OpenHantek'>https://github.com/OpenHantek</a></p>"
               "<p>Open source firmware copyright &copy; 2019-2020 Ho-Ro<br/>"
               "<a href='https://github.com/Ho-Ro/Hantek6022API'>https://github.com/Ho-Ro/Hantek6022API</a></p>") +
               tr("<p>Running since %1 seconds.</p>"
            ).arg( QDateTime::currentSecsSinceEpoch() - startDateTime.toSecsSinceEpoch()  )
        );
    });

    emit settingsLoaded(&dsoSettings->scope, spec);

    dsoWidget->updateTimebase(dsoSettings->scope.horizontal.timebase);

    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        this->dsoWidget->updateVoltageUsed(channel, dsoSettings->scope.voltage[channel].used);
        this->dsoWidget->updateSpectrumUsed(channel, dsoSettings->scope.spectrum[channel].used);
    }
}

MainWindow::~MainWindow() { 
    delete ui;
}

void MainWindow::showNewData(std::shared_ptr<PPresult> newData) {
    dsoWidget->showNew( newData );
}

void MainWindow::exporterStatusChanged(const QString &exporterName, const QString &status) {
    ui->statusbar->showMessage(tr("%1: %2").arg(exporterName, status));
}

void MainWindow::exporterProgressChanged() { 
    exporterRegistry->checkForWaitingExporters();
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void MainWindow::closeEvent(QCloseEvent *event) {
    if (dsoSettings->alwaysSave) {
        dsoSettings->mainWindowGeometry = saveGeometry();
        dsoSettings->mainWindowState = saveState();
        dsoSettings->save();
    }
    QMainWindow::closeEvent(event);
}
