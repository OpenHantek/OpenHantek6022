#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "HorizontalDock.h"
#include "SpectrumDock.h"
#include "TriggerDock.h"
#include "VoltageDock.h"
#include "dockwindows.h"
#include "exporter.h"

#include "configdialog.h"
#include "analyse/dataanalyzer.h"
#include "dockwindows.h"
#include "dsowidget.h"
#include "hantekdsocontrol.h"
#include "usb/usbdevice.h"
#include "dsomodel.h"
#include "viewconstants.h"

#include "settings.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>

MainWindow::MainWindow(HantekDsoControl *dsoControl, DataAnalyzer *dataAnalyser, DsoSettings *settings, QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), dsoControl(dsoControl), dataAnalyzer(dataAnalyser), settings(settings)
{
    ui->setupUi(this);


    // Window title
    setWindowIcon(QIcon(":openhantek.png"));
    setWindowTitle(tr("OpenHantek - Device %1").arg(QString::fromStdString(dsoControl->getDevice()->getModel()->name)));

// Create dock windows before the dso widget, they fix messed up settings
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif

    registerDockMetaTypes();
    horizontalDock = new HorizontalDock(&settings->scope, this);
    triggerDock = new TriggerDock(&settings->scope, settings->deviceSpecification, this);
    spectrumDock = new SpectrumDock(&settings->scope, this);
    voltageDock = new VoltageDock(&settings->scope, settings->deviceSpecification, this);

    // Central oszilloscope widget
    dsoWidget = new DsoWidget(&settings->scope, &settings->view, settings->deviceSpecification);
    connect(dataAnalyzer, &DataAnalyzer::analyzed,
            [this]() { dsoWidget->showNewData(this->dataAnalyzer->getNextResult()); });
    setCentralWidget(dsoWidget);

    addDockWidget(Qt::RightDockWidgetArea, horizontalDock);
    addDockWidget(Qt::RightDockWidgetArea, triggerDock);
    addDockWidget(Qt::RightDockWidgetArea, voltageDock);
    addDockWidget(Qt::RightDockWidgetArea, spectrumDock);

    restoreGeometry(settings->mainWindowGeometry);
    restoreState(settings->mainWindowState);

    // Command field inside the status bar
    QLineEdit* commandEdit = new QLineEdit(this);
    commandEdit->hide();

    statusBar()->addPermanentWidget(commandEdit, 1);

    connect(ui->actionManualCommand, &QAction::toggled, [this, commandEdit](bool checked) {
        commandEdit->setVisible(checked);
        if (checked)
            commandEdit->setFocus();
    });

    connect(commandEdit, &QLineEdit::returnPressed, [this, commandEdit]() {
        Dso::ErrorCode errorCode = this->dsoControl->stringCommand(commandEdit->text());
        commandEdit->clear();
        this->ui->actionManualCommand->setChecked(false);
        if (errorCode != Dso::ErrorCode::NONE) statusBar()->showMessage(tr("Invalid command"), 3000);
    });

    // Connect general signals
    connect(dsoControl, &HantekDsoControl::statusMessage, statusBar(), &QStatusBar::showMessage);

    // Connect signals to DSO controller and widget
    connect(horizontalDock, &HorizontalDock::samplerateChanged, [this]() {
        this->dsoControl->setSamplerate(this->settings->scope.horizontal.samplerate);
        this->dsoWidget->updateSamplerate(this->settings->scope.horizontal.samplerate);
    });
    connect(horizontalDock, &HorizontalDock::timebaseChanged, [this](){
        this->dsoControl->setRecordTime(this->settings->scope.horizontal.timebase * DIVS_TIME);
        this->dsoWidget->updateTimebase(this->settings->scope.horizontal.timebase);
    });
    connect(horizontalDock, &HorizontalDock::frequencybaseChanged, dsoWidget, &DsoWidget::updateFrequencybase);
    connect(horizontalDock, &HorizontalDock::recordLengthChanged, [this](unsigned long recordLength) {
        this->dsoControl->setRecordLength(recordLength);
    });

    connect(dsoControl, &HantekDsoControl::recordTimeChanged, [this](double duration) {
        if (this->settings->scope.horizontal.samplerateSource == DsoSettingsScopeHorizontal::Samplerrate &&
                this->settings->scope.horizontal.recordLength != UINT_MAX) {
            // The samplerate was set, let's adapt the timebase accordingly
            this->settings->scope.horizontal.timebase = horizontalDock->setTimebase(duration / DIVS_TIME);
        }

        // The trigger position should be kept at the same place but the timebase has
        // changed
        this->dsoControl->setPretriggerPosition(this->settings->scope.trigger.position * this->settings->scope.horizontal.timebase *
                                          DIVS_TIME);

        dsoWidget->updateTimebase(this->settings->scope.horizontal.timebase);
    });
    connect(dsoControl, &HantekDsoControl::samplerateChanged, [this](double samplerate) {
        if (this->settings->scope.horizontal.samplerateSource == DsoSettingsScopeHorizontal::Duration &&
                this->settings->scope.horizontal.recordLength != UINT_MAX) {
            // The timebase was set, let's adapt the samplerate accordingly
            this->settings->scope.horizontal.samplerate = samplerate;
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

    auto usedChanged = [this](ChannelID channel, bool used) {
        if (channel >= (unsigned int)this->settings->scope.voltage.size()) return;

        bool mathUsed = this->settings->scope.anyUsed(this->settings->deviceSpecification->channels);

        // Normal channel, check if voltage/spectrum or math channel is used
        if (channel < this->settings->deviceSpecification->channels)
            this->dsoControl->setChannelUsed(
                channel, mathUsed | this->settings->scope.anyUsed(channel));
        // Math channel, update all channels
        else if (channel == this->settings->deviceSpecification->channels) {
            for (ChannelID c = 0; c < this->settings->deviceSpecification->channels; ++c)
                this->dsoControl->setChannelUsed(c, mathUsed | this->settings->scope.anyUsed(c));
        }
    };
    connect(voltageDock, &VoltageDock::usedChanged, usedChanged);
    connect(spectrumDock, &SpectrumDock::usedChanged, usedChanged);

    connect(voltageDock, &VoltageDock::couplingChanged, dsoControl, &HantekDsoControl::setCoupling);
    connect(voltageDock, &VoltageDock::couplingChanged, dsoWidget, &DsoWidget::updateVoltageCoupling);
    connect(voltageDock, &VoltageDock::modeChanged, dsoWidget, &DsoWidget::updateMathMode);
    connect(voltageDock, &VoltageDock::gainChanged, [this](ChannelID channel, double gain) {
        if (channel >= this->settings->deviceSpecification->channels) return;

        this->dsoControl->setGain(channel, this->settings->scope.gain(channel) * DIVS_VOLTAGE);
    });
    connect(voltageDock, &VoltageDock::gainChanged, dsoWidget, &DsoWidget::updateVoltageGain);
    connect(dsoWidget, &DsoWidget::offsetChanged, [this](ChannelID channel) {
        if (channel >= this->settings->deviceSpecification->channels) return;
        this->dsoControl->setOffset(channel, (this->settings->scope.voltage[channel].offset / DIVS_VOLTAGE) + 0.5);
    });

    connect(voltageDock, &VoltageDock::usedChanged, dsoWidget, &DsoWidget::updateVoltageUsed);
    connect(spectrumDock, &SpectrumDock::usedChanged, dsoWidget, &DsoWidget::updateSpectrumUsed);
    connect(spectrumDock, &SpectrumDock::magnitudeChanged, dsoWidget, &DsoWidget::updateSpectrumMagnitude);

    // Started/stopped signals from oscilloscope
    connect(dsoControl, &HantekDsoControl::samplingStarted, [this]() {
        this->ui->actionSampling->setText(tr("&Stop"));
        this->ui->actionSampling->setIcon(QIcon(":actions/stop.png"));
        this->ui->actionSampling->setStatusTip(tr("Stop the oscilloscope"));

        disconnect(this->ui->actionSampling, &QAction::triggered, this->dsoControl, &HantekDsoControl::startSampling);
        connect(this->ui->actionSampling, &QAction::triggered, this->dsoControl, &HantekDsoControl::stopSampling);
    });
    connect(dsoControl, &HantekDsoControl::samplingStopped, [this]() {
        this->ui->actionSampling->setText(tr("&Start"));
        this->ui->actionSampling->setIcon(QIcon(":actions/start.png"));
        this->ui->actionSampling->setStatusTip(tr("Start the oscilloscope"));

        disconnect(this->ui->actionSampling, &QAction::triggered, this->dsoControl, &HantekDsoControl::stopSampling);
        connect(this->ui->actionSampling, &QAction::triggered, this->dsoControl, &HantekDsoControl::startSampling);
    });

    connect(dsoControl, &HantekDsoControl::availableRecordLengthsChanged, horizontalDock,
            &HorizontalDock::setAvailableRecordLengths);
    connect(dsoControl, &HantekDsoControl::samplerateLimitsChanged, horizontalDock,
            &HorizontalDock::setSamplerateLimits);
    connect(dsoControl, &HantekDsoControl::samplerateSet, horizontalDock, &HorizontalDock::setSamplerateSteps);

    connect(ui->actionOpen, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr("Settings (*.ini)"));
        if (!fileName.isEmpty()) {
            if (this->settings->setFilename(fileName)) {
                this->settings->load();
            }
        }
    });

    connect(ui->actionSave, &QAction::triggered, [this]() {
        this->settings->mainWindowGeometry = saveGeometry();
        this->settings->mainWindowState = saveState();
        this->settings->save();
    });

    connect(ui->actionSave_as, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings"), "", tr("Settings (*.ini)"));
        if (fileName.isEmpty()) return;
        this->settings->mainWindowGeometry = saveGeometry();
        this->settings->mainWindowState = saveState();
        this->settings->setFilename(fileName);
        this->settings->save();
    });

    connect(ui->actionPrint, &QAction::triggered, [this]() {
        this->dsoWidget->setExporterForNextFrame(std::unique_ptr<Exporter>(Exporter::createPrintExporter(this->settings)));
    });

    connect(ui->actionExport, &QAction::triggered, [this]() {
        this->dsoWidget->setExporterForNextFrame(std::unique_ptr<Exporter>(Exporter::createSaveToFileExporter(this->settings)));
    });

    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionSettings, &QAction::triggered, [this]() {
        this->settings->mainWindowGeometry = saveGeometry();
        this->settings->mainWindowState = saveState();

        DsoConfigDialog* configDialog = new DsoConfigDialog(this->settings, this);
        configDialog->setModal(true);
        configDialog->show();
    });

    connect(this->ui->actionDigital_phosphor, &QAction::toggled, [this](bool enabled) {
        this->settings->view.digitalPhosphor = enabled;

        if (this->settings->view.digitalPhosphor)
            this->ui->actionDigital_phosphor->setStatusTip(tr("Disable fading of previous graphs"));
        else
            this->ui->actionDigital_phosphor->setStatusTip(tr("Enable fading of previous graphs"));
    });
    this->ui->actionDigital_phosphor->setChecked(settings->view.digitalPhosphor);

    connect(ui->actionZoom, &QAction::toggled, [this](bool enabled) {
        this->settings->view.zoom = enabled;

        if (this->settings->view.zoom)
            this->ui->actionZoom->setStatusTip(tr("Hide magnified scope"));
        else
            this->ui->actionZoom->setStatusTip(tr("Show magnified scope"));

        this->dsoWidget->updateZoom(enabled);
    });
    ui->actionZoom->setChecked(settings->view.zoom);

    connect(ui->actionAbout, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr("About OpenHantek %1").arg(VERSION),
            tr("<p>This is a open source software for Hantek USB oscilloscopes.</p>"
               "<p>Copyright &copy; 2010, 2011 Oliver Haag<br><a "
               "href='mailto:oliver.haag@gmail.com'>oliver.haag@gmail.com</a></p>"
               "<p>Copyright &copy; 2012-2017 OpenHantek community<br>"
               "<a href='https://github.com/OpenHantek/openhantek'>https://github.com/OpenHantek/openhantek</a></p>"));

    });

    if (settings->scope.horizontal.samplerateSource == DsoSettingsScopeHorizontal::Samplerrate)
        dsoWidget->updateSamplerate(settings->scope.horizontal.samplerate);
    else
        dsoWidget->updateTimebase(settings->scope.horizontal.timebase);

    for (ChannelID channel = 0; channel < settings->deviceSpecification->channels; ++channel) {
        this->dsoWidget->updateVoltageUsed(channel, settings->scope.voltage[channel].used);
        this->dsoWidget->updateSpectrumUsed(channel, settings->scope.spectrum[channel].used);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void MainWindow::closeEvent(QCloseEvent *event) {
    if (settings->options.alwaysSave) {
        settings->mainWindowGeometry = saveGeometry();
        settings->mainWindowState = saveState();
        settings->save();
    }

    QMainWindow::closeEvent(event);
}
