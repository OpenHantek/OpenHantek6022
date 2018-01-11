#include "mainwindow.h"
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
#include "hantekdsocontrol.h"
#include "usb/usbdevice.h"
#include "viewconstants.h"

#include "settings.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>

MainWindow::MainWindow(HantekDsoControl *dsoControl, DsoSettings *settings, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), mSettings(settings) {
    ui->setupUi(this);

    // Window title
    setWindowIcon(QIcon(":openhantek.png"));
    setWindowTitle(tr("OpenHantek - Device %1").arg(QString::fromStdString(dsoControl->getDevice()->getModel()->name)));

// Create dock windows before the dso widget, they fix messed up settings
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif

    DsoSettingsScope *scope = &(mSettings->scope);
    const Dso::ControlSpecification *spec = &dsoControl->getDevice()->getModel()->specification;

    registerDockMetaTypes();

    // Docking windows
    HorizontalDock *horizontalDock;
    TriggerDock *triggerDock;
    SpectrumDock *spectrumDock;
    VoltageDock *voltageDock;
    horizontalDock = new HorizontalDock(scope, this);
    triggerDock = new TriggerDock(scope, spec, this);
    spectrumDock = new SpectrumDock(scope, this);
    voltageDock = new VoltageDock(scope, spec, this);

    addDockWidget(Qt::RightDockWidgetArea, horizontalDock);
    addDockWidget(Qt::RightDockWidgetArea, triggerDock);
    addDockWidget(Qt::RightDockWidgetArea, voltageDock);
    addDockWidget(Qt::RightDockWidgetArea, spectrumDock);

    restoreGeometry(mSettings->mainWindowGeometry);
    restoreState(mSettings->mainWindowState);

    // Central oszilloscope widget
    dsoWidget = new DsoWidget(&mSettings->scope, &mSettings->view, spec);
    setCentralWidget(dsoWidget);

    // Command field inside the status bar
    QLineEdit *commandEdit = new QLineEdit(this);
    commandEdit->hide();

    statusBar()->addPermanentWidget(commandEdit, 1);

    connect(ui->actionManualCommand, &QAction::toggled, [this, commandEdit](bool checked) {
        commandEdit->setVisible(checked);
        if (checked) commandEdit->setFocus();
    });

    connect(commandEdit, &QLineEdit::returnPressed, [this, commandEdit, dsoControl]() {
        Dso::ErrorCode errorCode = dsoControl->stringCommand(commandEdit->text());
        commandEdit->clear();
        this->ui->actionManualCommand->setChecked(false);
        if (errorCode != Dso::ErrorCode::NONE) statusBar()->showMessage(tr("Invalid command"), 3000);
    });

    // Connect general signals
    connect(dsoControl, &HantekDsoControl::statusMessage, statusBar(), &QStatusBar::showMessage);

    // Connect signals to DSO controller and widget
    connect(horizontalDock, &HorizontalDock::samplerateChanged, [dsoControl, this]() {
        dsoControl->setSamplerate(mSettings->scope.horizontal.samplerate);
        this->dsoWidget->updateSamplerate(mSettings->scope.horizontal.samplerate);
    });
    connect(horizontalDock, &HorizontalDock::timebaseChanged, [dsoControl, this]() {
        dsoControl->setRecordTime(mSettings->scope.horizontal.timebase * DIVS_TIME);
        this->dsoWidget->updateTimebase(mSettings->scope.horizontal.timebase);
    });
    connect(horizontalDock, &HorizontalDock::frequencybaseChanged, dsoWidget, &DsoWidget::updateFrequencybase);
    connect(horizontalDock, &HorizontalDock::recordLengthChanged,
            [dsoControl](unsigned long recordLength) { dsoControl->setRecordLength(recordLength); });

    connect(dsoControl, &HantekDsoControl::recordTimeChanged,
            [this, settings, horizontalDock, dsoControl](double duration) {
                if (settings->scope.horizontal.samplerateSource == DsoSettingsScopeHorizontal::Samplerrate &&
                    settings->scope.horizontal.recordLength != UINT_MAX) {
                    // The samplerate was set, let's adapt the timebase accordingly
                    settings->scope.horizontal.timebase = horizontalDock->setTimebase(duration / DIVS_TIME);
                }

                // The trigger position should be kept at the same place but the timebase has
                // changed
                dsoControl->setPretriggerPosition(settings->scope.trigger.position *
                                                  settings->scope.horizontal.timebase * DIVS_TIME);

                this->dsoWidget->updateTimebase(settings->scope.horizontal.timebase);
            });
    connect(dsoControl, &HantekDsoControl::samplerateChanged, [this, horizontalDock](double samplerate) {
        if (mSettings->scope.horizontal.samplerateSource == DsoSettingsScopeHorizontal::Duration &&
            mSettings->scope.horizontal.recordLength != UINT_MAX) {
            // The timebase was set, let's adapt the samplerate accordingly
            mSettings->scope.horizontal.samplerate = samplerate;
            horizontalDock->setSamplerate(samplerate);
            dsoWidget->updateSamplerate(samplerate);
        }
    });

    connect(triggerDock, &TriggerDock::modeChanged, dsoControl, &HantekDsoControl::setTriggerMode);
    connect(triggerDock, &TriggerDock::modeChanged, dsoWidget, &DsoWidget::updateTriggerMode);
    connect(triggerDock, &TriggerDock::sourceChanged, dsoControl, &HantekDsoControl::setTriggerSource);
    connect(triggerDock, &TriggerDock::sourceChanged, dsoWidget, &DsoWidget::updateTriggerSource);
    connect(triggerDock, &TriggerDock::slopeChanged, dsoControl, &HantekDsoControl::setTriggerSlope);
    connect(triggerDock, &TriggerDock::slopeChanged, dsoWidget, &DsoWidget::updateTriggerSlope);
    connect(dsoWidget, &DsoWidget::triggerPositionChanged, dsoControl, &HantekDsoControl::setPretriggerPosition);
    connect(dsoWidget, &DsoWidget::triggerLevelChanged, dsoControl, &HantekDsoControl::setTriggerLevel);

    auto usedChanged = [this, dsoControl, spec](ChannelID channel, bool used) {
        if (channel >= (unsigned int)mSettings->scope.voltage.size()) return;

        bool mathUsed = mSettings->scope.anyUsed(spec->channels);

        // Normal channel, check if voltage/spectrum or math channel is used
        if (channel < spec->channels)
            dsoControl->setChannelUsed(channel, mathUsed | mSettings->scope.anyUsed(channel));
        // Math channel, update all channels
        else if (channel == spec->channels) {
            for (ChannelID c = 0; c < spec->channels; ++c)
                dsoControl->setChannelUsed(c, mathUsed | mSettings->scope.anyUsed(c));
        }
    };
    connect(voltageDock, &VoltageDock::usedChanged, usedChanged);
    connect(spectrumDock, &SpectrumDock::usedChanged, usedChanged);

    connect(voltageDock, &VoltageDock::couplingChanged, dsoControl, &HantekDsoControl::setCoupling);
    connect(voltageDock, &VoltageDock::couplingChanged, dsoWidget, &DsoWidget::updateVoltageCoupling);
    connect(voltageDock, &VoltageDock::modeChanged, dsoWidget, &DsoWidget::updateMathMode);
    connect(voltageDock, &VoltageDock::gainChanged, [this, dsoControl, spec](ChannelID channel, double gain) {
        if (channel >= spec->channels) return;

        dsoControl->setGain(channel, mSettings->scope.gain(channel) * DIVS_VOLTAGE);
    });
    connect(voltageDock, &VoltageDock::gainChanged, dsoWidget, &DsoWidget::updateVoltageGain);
    connect(dsoWidget, &DsoWidget::offsetChanged, [this, dsoControl, spec](ChannelID channel) {
        if (channel >= spec->channels) return;
        dsoControl->setOffset(channel, (mSettings->scope.voltage[channel].offset / DIVS_VOLTAGE) + 0.5);
    });

    connect(voltageDock, &VoltageDock::usedChanged, dsoWidget, &DsoWidget::updateVoltageUsed);
    connect(spectrumDock, &SpectrumDock::usedChanged, dsoWidget, &DsoWidget::updateSpectrumUsed);
    connect(spectrumDock, &SpectrumDock::magnitudeChanged, dsoWidget, &DsoWidget::updateSpectrumMagnitude);

    // Started/stopped signals from oscilloscope
    connect(dsoControl, &HantekDsoControl::samplingStarted, [this, dsoControl]() {
        this->ui->actionSampling->setText(tr("&Stop"));
        this->ui->actionSampling->setIcon(QIcon(":actions/stop.png"));
        this->ui->actionSampling->setStatusTip(tr("Stop the oscilloscope"));

        disconnect(this->ui->actionSampling, &QAction::triggered, dsoControl, &HantekDsoControl::startSampling);
        connect(this->ui->actionSampling, &QAction::triggered, dsoControl, &HantekDsoControl::stopSampling);
    });
    connect(dsoControl, &HantekDsoControl::samplingStopped, [this, dsoControl]() {
        this->ui->actionSampling->setText(tr("&Start"));
        this->ui->actionSampling->setIcon(QIcon(":actions/start.png"));
        this->ui->actionSampling->setStatusTip(tr("Start the oscilloscope"));

        disconnect(this->ui->actionSampling, &QAction::triggered, dsoControl, &HantekDsoControl::stopSampling);
        connect(this->ui->actionSampling, &QAction::triggered, dsoControl, &HantekDsoControl::startSampling);
    });

    connect(dsoControl, &HantekDsoControl::availableRecordLengthsChanged, horizontalDock,
            &HorizontalDock::setAvailableRecordLengths);
    connect(dsoControl, &HantekDsoControl::samplerateLimitsChanged, horizontalDock,
            &HorizontalDock::setSamplerateLimits);
    connect(dsoControl, &HantekDsoControl::samplerateSet, horizontalDock, &HorizontalDock::setSamplerateSteps);

    connect(ui->actionOpen, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr("Settings (*.ini)"));
        if (!fileName.isEmpty()) {
            if (mSettings->setFilename(fileName)) { mSettings->load(); }
        }
    });

    connect(ui->actionSave, &QAction::triggered, [this]() {
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();
        mSettings->save();
    });

    connect(ui->actionSave_as, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings"), "", tr("Settings (*.ini)"));
        if (fileName.isEmpty()) return;
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();
        mSettings->setFilename(fileName);
        mSettings->save();
    });

    connect(ui->actionPrint, &QAction::triggered, [this, spec]() {
        this->dsoWidget->setExporterForNextFrame(
            std::unique_ptr<Exporter>(Exporter::createPrintExporter(spec, this->mSettings)));
    });

    connect(ui->actionExport, &QAction::triggered, [this, spec]() {
        this->dsoWidget->setExporterForNextFrame(
            std::unique_ptr<Exporter>(Exporter::createSaveToFileExporter(spec, this->mSettings)));
    });

    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionSettings, &QAction::triggered, [this]() {
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();

        DsoConfigDialog *configDialog = new DsoConfigDialog(this->mSettings, this);
        configDialog->setModal(true);
        configDialog->show();
    });

    connect(this->ui->actionDigital_phosphor, &QAction::toggled, [this](bool enabled) {
        mSettings->view.digitalPhosphor = enabled;

        if (mSettings->view.digitalPhosphor)
            this->ui->actionDigital_phosphor->setStatusTip(tr("Disable fading of previous graphs"));
        else
            this->ui->actionDigital_phosphor->setStatusTip(tr("Enable fading of previous graphs"));
    });
    this->ui->actionDigital_phosphor->setChecked(mSettings->view.digitalPhosphor);

    connect(ui->actionZoom, &QAction::toggled, [this](bool enabled) {
        mSettings->view.zoom = enabled;

        if (mSettings->view.zoom)
            this->ui->actionZoom->setStatusTip(tr("Hide magnified scope"));
        else
            this->ui->actionZoom->setStatusTip(tr("Show magnified scope"));

        this->dsoWidget->updateZoom(enabled);
    });
    ui->actionZoom->setChecked(mSettings->view.zoom);

    connect(ui->actionAbout, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr("About OpenHantek %1").arg(VERSION),
            tr("<p>This is a open source software for Hantek USB oscilloscopes.</p>"
               "<p>Copyright &copy; 2010, 2011 Oliver Haag<br><a "
               "href='mailto:oliver.haag@gmail.com'>oliver.haag@gmail.com</a></p>"
               "<p>Copyright &copy; 2012-2017 OpenHantek community<br>"
               "<a href='https://github.com/OpenHantek/openhantek'>https://github.com/OpenHantek/openhantek</a></p>"));

    });

    if (mSettings->scope.horizontal.samplerateSource == DsoSettingsScopeHorizontal::Samplerrate)
        dsoWidget->updateSamplerate(mSettings->scope.horizontal.samplerate);
    else
        dsoWidget->updateTimebase(mSettings->scope.horizontal.timebase);

    for (ChannelID channel = 0; channel < spec->channels; ++channel) {
        this->dsoWidget->updateVoltageUsed(channel, mSettings->scope.voltage[channel].used);
        this->dsoWidget->updateSpectrumUsed(channel, mSettings->scope.spectrum[channel].used);
    }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::showNewData(std::shared_ptr<PPresult> data) { dsoWidget->showNew(data); }

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void MainWindow::closeEvent(QCloseEvent *event) {
    if (mSettings->alwaysSave) {
        mSettings->mainWindowGeometry = saveGeometry();
        mSettings->mainWindowState = saveState();
        mSettings->save();
    }

    QMainWindow::closeEvent(event);
}
