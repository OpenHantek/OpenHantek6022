// SPDX-License-Identifier: GPL-2.0+

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include "mainwindow.h"

#include "configdialog.h"
#include "dataanalyzer.h"
#include "dockwindows.h"
#include "hantekdsocontrol.h"
#include "usb/usbdevice.h"
#include "dsowidget.h"
#include "hantek/hantekdsocontrol.h"
#include "settings.h"

////////////////////////////////////////////////////////////////////////////////
// class OpenHantekMainWindow
/// \brief Initializes the gui elements of the main window.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
OpenHantekMainWindow::OpenHantekMainWindow(std::shared_ptr<HantekDsoControl> dsoControl, std::shared_ptr<DataAnalyzer> dataAnalyzer)
    :dsoControl(dsoControl),dataAnalyzer(dataAnalyzer) {

    // Window title
    setWindowIcon(QIcon(":openhantek.png"));
    setWindowTitle(tr("OpenHantek - Device %1").arg(QString::fromStdString(dsoControl->getDevice()->getModel().name)));

    // Application settings
    settings = new DsoSettings();
    settings->setChannelCount(dsoControl->getChannelCount());
    readSettings();

    // Create dock windows before the dso widget, they fix messed up settings
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
    createDockWindows();

    // Central oszilloscope widget
    dsoWidget = new DsoWidget(settings, dataAnalyzer.get());
    setCentralWidget(dsoWidget);

    // Subroutines for window elements
    createActions();
    createToolBars();
    createMenus();
    createStatusBar();

    // Apply the settings after the gui is initialized
    applySettings();

    // Connect all signals
    connectSignals();

    // Set up the oscilloscope
    applySettingsToDevice();
    dsoControl->startSampling();
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void OpenHantekMainWindow::closeEvent(QCloseEvent *event) {
    if (settings->options.alwaysSave)
        writeSettings();

    QMainWindow::closeEvent(event);
}

/// \brief Create the used actions.
void OpenHantekMainWindow::createActions() {
    openAction =
            new QAction(QIcon(":actions/open.png"), tr("&Open..."), this);
    openAction->setShortcut(tr("Ctrl+O"));
    openAction->setStatusTip(tr("Open saved settings"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    saveAction = new QAction(QIcon(":actions/save.png"), tr("&Save"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setStatusTip(tr("Save the current settings"));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAction =
            new QAction(QIcon(":actions/save-as.png"), tr("Save &as..."), this);
    saveAsAction->setStatusTip(
                tr("Save the current settings to another file"));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

    printAction =
            new QAction(QIcon(":actions/print.png"), tr("&Print..."), this);
    printAction->setShortcut(tr("Ctrl+P"));
    printAction->setStatusTip(tr("Print the oscilloscope screen"));
    connect(printAction, SIGNAL(triggered()), dsoWidget,
            SLOT(print()));

    exportAsAction =
            new QAction(QIcon(":actions/export-as.png"), tr("&Export as..."), this);
    exportAsAction->setShortcut(tr("Ctrl+E"));
    exportAsAction->setStatusTip(
                tr("Export the oscilloscope data to a file"));
    connect(exportAsAction, SIGNAL(triggered()), dsoWidget,
            SLOT(exportAs()));

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    configAction = new QAction(tr("&Settings"), this);
    configAction->setShortcut(tr("Ctrl+S"));
    configAction->setStatusTip(tr("Configure the oscilloscope"));
    connect(configAction, SIGNAL(triggered()), this, SLOT(config()));

    startStopAction = new QAction(this);
    startStopAction->setShortcut(tr("Space"));
    stopped();

    digitalPhosphorAction = new QAction(
                QIcon(":actions/digitalphosphor.png"), tr("Digital &phosphor"), this);
    digitalPhosphorAction->setCheckable(true);
    digitalPhosphorAction->setChecked(settings->view.digitalPhosphor);
    digitalPhosphor(settings->view.digitalPhosphor);
    connect(digitalPhosphorAction, SIGNAL(toggled(bool)), this,
            SLOT(digitalPhosphor(bool)));

    zoomAction = new QAction(QIcon(":actions/zoom.png"), tr("&Zoom"), this);
    zoomAction->setCheckable(true);
    zoomAction->setChecked(settings->view.zoom);
    zoom(settings->view.zoom);
    connect(zoomAction, SIGNAL(toggled(bool)), this, SLOT(zoom(bool)));
    connect(zoomAction, SIGNAL(toggled(bool)), dsoWidget,
            SLOT(updateZoom(bool)));

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show information about this program"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

#ifdef DEBUG
    commandAction = new QAction(tr("Send command"), this);
    commandAction->setShortcut(tr("Shift+C"));
#endif
}

/// \brief Create the menus and menuitems.
void OpenHantekMainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(printAction);
    fileMenu->addAction(exportAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(digitalPhosphorAction);
    viewMenu->addAction(zoomAction);
    viewMenu->addSeparator();
    dockMenu = viewMenu->addMenu(tr("&Docking windows"));
    dockMenu->addAction(horizontalDock->toggleViewAction());
    dockMenu->addAction(spectrumDock->toggleViewAction());
    dockMenu->addAction(triggerDock->toggleViewAction());
    dockMenu->addAction(voltageDock->toggleViewAction());
    toolbarMenu = viewMenu->addMenu(tr("&Toolbars"));
    toolbarMenu->addAction(fileToolBar->toggleViewAction());
    toolbarMenu->addAction(oscilloscopeToolBar->toggleViewAction());
    toolbarMenu->addAction(viewToolBar->toggleViewAction());

    oscilloscopeMenu = menuBar()->addMenu(tr("&Oscilloscope"));
    oscilloscopeMenu->addAction(configAction);
    oscilloscopeMenu->addSeparator();
    oscilloscopeMenu->addAction(startStopAction);
#ifdef DEBUG
    oscilloscopeMenu->addSeparator();
    oscilloscopeMenu->addAction(commandAction);
#endif

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(aboutQtAction);
}

namespace {

QToolBar* CreateToolBar(const QString& title) {
    QToolBar* newObj = new QToolBar(title);
    newObj->setObjectName(title);
    newObj->setAllowedAreas(Qt::TopToolBarArea | Qt::LeftToolBarArea);
    return newObj;
}

}

/// \brief Create the toolbars and their buttons.
void OpenHantekMainWindow::createToolBars() {

    // File
    fileToolBar = CreateToolBar(tr("File"));
    fileToolBar->addAction(openAction);
    fileToolBar->addAction(saveAction);
    fileToolBar->addAction(saveAsAction);
    fileToolBar->addSeparator();
    fileToolBar->addAction(printAction);
    fileToolBar->addAction(exportAsAction);

    // Oscilloscope
    oscilloscopeToolBar = CreateToolBar(tr("Oscilloscope"));
    oscilloscopeToolBar->addAction(startStopAction);

    // View
    viewToolBar = CreateToolBar(tr("View"));
    viewToolBar->addAction(digitalPhosphorAction);
    viewToolBar->addAction(zoomAction);
}

/// \brief Create the status bar.
void OpenHantekMainWindow::createStatusBar() {
#ifdef DEBUG
    // Command field inside the status bar
    commandEdit = new QLineEdit();
    commandEdit->hide();

    statusBar()->addPermanentWidget(commandEdit, 1);
#endif

    statusBar()->showMessage(tr("Ready"));

#ifdef DEBUG
    connect(commandAction, SIGNAL(triggered()), commandEdit,
            SLOT(show()));
    connect(commandAction, SIGNAL(triggered()), commandEdit,
            SLOT(setFocus()));
    connect(commandEdit, SIGNAL(returnPressed()), this,
            SLOT(sendCommand()));
#endif
}

/// \brief Create all docking windows.
void OpenHantekMainWindow::createDockWindows() {
    horizontalDock = new HorizontalDock(settings, this);
    triggerDock = new TriggerDock(settings, dsoControl->getSpecialTriggerSources(), this);
    spectrumDock = new SpectrumDock(settings, this);
    voltageDock = new VoltageDock(settings, this);
}

/// \brief Connect general signals and device management signals.
void OpenHantekMainWindow::connectSignals() {
    // Connect general signals
    connect(this, SIGNAL(settingsChanged()), this, SLOT(applySettings()));
    // connect(dsoWidget, SIGNAL(stopped()), this, SLOT(stopped()));
    connect(dsoControl.get(), SIGNAL(statusMessage(QString, int)),
            statusBar(), SLOT(showMessage(QString, int)));
    connect(dsoControl.get(),
            SIGNAL(samplesAvailable(const std::vector<std::vector<double>> *,
                                    double, bool, QMutex *)),
            dataAnalyzer.get(),
            SLOT(analyze(const std::vector<std::vector<double>> *, double, bool,
                         QMutex *)));

    // Connect signals to DSO controller and widget
    connect(horizontalDock, SIGNAL(samplerateChanged(double)), this,
            SLOT(samplerateSelected()));
    connect(horizontalDock, SIGNAL(timebaseChanged(double)), this,
            SLOT(timebaseSelected()));
    connect(horizontalDock, SIGNAL(frequencybaseChanged(double)),
            dsoWidget, SLOT(updateFrequencybase(double)));
    connect(horizontalDock, SIGNAL(recordLengthChanged(unsigned long)),
            this, SLOT(recordLengthSelected(unsigned long)));
    // connect(horizontalDock, SIGNAL(formatChanged(HorizontalFormat)),
    // dsoWidget, SLOT(horizontalFormatChanged(HorizontalFormat)));

    connect(triggerDock, SIGNAL(modeChanged(Dso::TriggerMode)),
            dsoControl.get(), SLOT(setTriggerMode(Dso::TriggerMode)));
    connect(triggerDock, SIGNAL(modeChanged(Dso::TriggerMode)),
            dsoWidget, SLOT(updateTriggerMode()));
    connect(triggerDock, SIGNAL(sourceChanged(bool, unsigned int)),
            dsoControl.get(), SLOT(setTriggerSource(bool, unsigned int)));
    connect(triggerDock, SIGNAL(sourceChanged(bool, unsigned int)),
            dsoWidget, SLOT(updateTriggerSource()));
    connect(triggerDock, SIGNAL(slopeChanged(Dso::Slope)), dsoControl.get(),
            SLOT(setTriggerSlope(Dso::Slope)));
    connect(triggerDock, SIGNAL(slopeChanged(Dso::Slope)), dsoWidget,
            SLOT(updateTriggerSlope()));
    connect(dsoWidget, SIGNAL(triggerPositionChanged(double)),
            dsoControl.get(), SLOT(setPretriggerPosition(double)));
    connect(dsoWidget, SIGNAL(triggerLevelChanged(unsigned int, double)),
            dsoControl.get(), SLOT(setTriggerLevel(unsigned int, double)));

    connect(voltageDock, SIGNAL(usedChanged(unsigned int, bool)), this,
            SLOT(updateUsed(unsigned int)));
    connect(voltageDock, SIGNAL(usedChanged(unsigned int, bool)),
            dsoWidget, SLOT(updateVoltageUsed(unsigned int, bool)));
    connect(voltageDock,
            SIGNAL(couplingChanged(unsigned int, Dso::Coupling)),
            dsoControl.get(), SLOT(setCoupling(unsigned int, Dso::Coupling)));
    connect(voltageDock,
            SIGNAL(couplingChanged(unsigned int, Dso::Coupling)), dsoWidget,
            SLOT(updateVoltageCoupling(unsigned int)));
    connect(voltageDock, SIGNAL(modeChanged(Dso::MathMode)),
            dsoWidget, SLOT(updateMathMode()));
    connect(voltageDock, SIGNAL(gainChanged(unsigned int, double)), this,
            SLOT(updateVoltageGain(unsigned int)));
    connect(voltageDock, SIGNAL(gainChanged(unsigned int, double)),
            dsoWidget, SLOT(updateVoltageGain(unsigned int)));
    connect(dsoWidget, SIGNAL(offsetChanged(unsigned int, double)), this,
            SLOT(updateOffset(unsigned int)));

    connect(spectrumDock, SIGNAL(usedChanged(unsigned int, bool)), this,
            SLOT(updateUsed(unsigned int)));
    connect(spectrumDock, SIGNAL(usedChanged(unsigned int, bool)),
            dsoWidget, SLOT(updateSpectrumUsed(unsigned int, bool)));
    connect(spectrumDock, SIGNAL(magnitudeChanged(unsigned int, double)),
            dsoWidget, SLOT(updateSpectrumMagnitude(unsigned int)));

    // Started/stopped signals from oscilloscope
    connect(dsoControl.get(), SIGNAL(samplingStarted()), this, SLOT(started()));
    connect(dsoControl.get(), SIGNAL(samplingStopped()), this, SLOT(stopped()));

    // connect(dsoControl, SIGNAL(recordLengthChanged(unsigned long)), this,
    // SLOT(recordLengthChanged()));
    connect(dsoControl.get(), SIGNAL(recordTimeChanged(double)), this,
            SLOT(recordTimeChanged(double)));
    connect(dsoControl.get(), SIGNAL(samplerateChanged(double)), this,
            SLOT(samplerateChanged(double)));

    connect(dsoControl.get(),
            SIGNAL(availableRecordLengthsChanged(QList<unsigned int>)),
            horizontalDock,
            SLOT(availableRecordLengthsChanged(QList<unsigned int>)));
    connect(dsoControl.get(), SIGNAL(samplerateLimitsChanged(double, double)),
            horizontalDock, SLOT(samplerateLimitsChanged(double, double)));
    connect(dsoControl.get(), SIGNAL(samplerateSet(int, QList<double>)),
            horizontalDock, SLOT(samplerateSet(int, QList<double>)));
}

/// \brief Initialize the device with the current settings.
void OpenHantekMainWindow::applySettingsToDevice() {
    for (unsigned int channel = 0;
         channel < settings->scope.physicalChannels; ++channel) {
        dsoControl->setCoupling(
                    channel, (Dso::Coupling)settings->scope.voltage[channel].misc);
        updateVoltageGain(channel);
        updateOffset(channel);
        dsoControl->setTriggerLevel(
                    channel, settings->scope.voltage[channel].trigger);
    }
    updateUsed(settings->scope.physicalChannels);
    if (settings->scope.horizontal.samplerateSet)
        samplerateSelected();
    else
        timebaseSelected();
    if (dsoControl->getAvailableRecordLengths()->isEmpty())
        dsoControl->setRecordLength(
                    settings->scope.horizontal.recordLength);
    else {
        int index = dsoControl->getAvailableRecordLengths()->indexOf(
                    settings->scope.horizontal.recordLength);
        dsoControl->setRecordLength(index < 0 ? 1 : index);
    }
    dsoControl->setTriggerMode(settings->scope.trigger.mode);
    dsoControl->setPretriggerPosition(
                settings->scope.trigger.position *
                settings->scope.horizontal.timebase * DIVS_TIME);
    dsoControl->setTriggerSlope(settings->scope.trigger.slope);
    dsoControl->setTriggerSource(settings->scope.trigger.special,
                                 settings->scope.trigger.source);
}

/// \brief Read the settings from an ini file.
/// \param fileName Optional filename to export the settings to a specific file.
/// \return 0 on success, negative on error.
int OpenHantekMainWindow::readSettings(const QString &fileName) {
    int status = settings->load(fileName);

    if (status == 0)
        emit(settingsChanged());

    return status;
}

/// \brief Save the settings to the harddisk.
/// \param fileName Optional filename to read the settings from an ini file.
/// \return 0 on success, negative on error.
int OpenHantekMainWindow::writeSettings(const QString &fileName) {
    updateSettings();

    return settings->save(fileName);
}

/// \brief Open a configuration file.
/// \return 0 on success, 1 on user abort, negative on error.
int OpenHantekMainWindow::open() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "",
                                                    tr("Settings (*.ini)"));

    if (!fileName.isEmpty())
        return readSettings(fileName);
    else
        return 1;
}

/// \brief Save the current configuration to a file.
/// \return 0 on success, negative on error.
int OpenHantekMainWindow::save() {
    if (currentFile.isEmpty())
        return saveAs();
    else
        return writeSettings(currentFile);
}

/// \brief Save the configuration to another filename.
/// \return 0 on success, 1 on user abort, negative on error.
int OpenHantekMainWindow::saveAs() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings"), "",
                                                    tr("Settings (*.ini)"));
    if (fileName.isEmpty())
        return 1;

    int status = writeSettings(fileName);

    if (status == 0)
        currentFile = fileName;

    return status;
}

/// \brief The oscilloscope started sampling.
void OpenHantekMainWindow::started() {
    startStopAction->setText(tr("&Stop"));
    startStopAction->setIcon(QIcon(":actions/stop.png"));
    startStopAction->setStatusTip(tr("Stop the oscilloscope"));

    disconnect(startStopAction, SIGNAL(triggered()), dsoControl.get(),
               SLOT(startSampling()));
    connect(startStopAction, SIGNAL(triggered()), dsoControl.get(),
            SLOT(stopSampling()));
}

/// \brief The oscilloscope stopped sampling.
void OpenHantekMainWindow::stopped() {
    startStopAction->setText(tr("&Start"));
    startStopAction->setIcon(QIcon(":actions/start.png"));
    startStopAction->setStatusTip(tr("Start the oscilloscope"));

    disconnect(startStopAction, SIGNAL(triggered()), dsoControl.get(),
               SLOT(stopSampling()));
    connect(startStopAction, SIGNAL(triggered()), dsoControl.get(),
            SLOT(startSampling()));
}

/// \brief Configure the oscilloscope.
void OpenHantekMainWindow::config() {
    updateSettings();

    DsoConfigDialog configDialog(settings, this);
    if (configDialog.exec() == QDialog::Accepted)
        settingsChanged();
}

/// \brief Enable/disable digital phosphor.
void OpenHantekMainWindow::digitalPhosphor(bool enabled) {
    settings->view.digitalPhosphor = enabled;

    if (settings->view.digitalPhosphor)
        digitalPhosphorAction->setStatusTip(
                    tr("Disable fading of previous graphs"));
    else
        digitalPhosphorAction->setStatusTip(
                    tr("Enable fading of previous graphs"));
}

/// \brief Show/hide the magnified scope.
void OpenHantekMainWindow::zoom(bool enabled) {
    settings->view.zoom = enabled;

    if (settings->view.zoom)
        zoomAction->setStatusTip(tr("Hide magnified scope"));
    else
        zoomAction->setStatusTip(tr("Show magnified scope"));
}

/// \brief Show the about dialog.
void OpenHantekMainWindow::about() {
    QMessageBox::about(
                this, tr("About OpenHantek %1").arg(VERSION),
                tr("<p>This is a open source software for Hantek USB oscilloscopes.</p>"
                   "<p>Copyright &copy; 2010, 2011 Oliver Haag "
                   "&lt;oliver.haag@gmail.com&gt;</p>"));
}

/// \brief The settings have changed.
void OpenHantekMainWindow::applySettings() {
    addDockWidget(Qt::RightDockWidgetArea, horizontalDock);
    addDockWidget(Qt::RightDockWidgetArea, triggerDock);
    addDockWidget(Qt::RightDockWidgetArea, voltageDock);
    addDockWidget(Qt::RightDockWidgetArea, spectrumDock);

    addToolBar(fileToolBar);
    addToolBar(oscilloscopeToolBar);
    addToolBar(viewToolBar);

    restoreGeometry(settings->mainWindowGeometry);
    restoreState(settings->mainWindowState);
}

/// \brief Update the window layout in the settings.
void OpenHantekMainWindow::updateSettings() {
    // Main window
    settings->mainWindowGeometry = saveGeometry();
    settings->mainWindowState = saveState();
}

/// \brief The oscilloscope changed the record time.
/// \param duration The new record time duration in seconds.
void OpenHantekMainWindow::recordTimeChanged(double duration) {
    if (settings->scope.horizontal.samplerateSet &&
            settings->scope.horizontal.recordLength != UINT_MAX) {
        // The samplerate was set, let's adapt the timebase accordingly
        settings->scope.horizontal.timebase =
                horizontalDock->setTimebase(duration / DIVS_TIME);
    }

    // The trigger position should be kept at the same place but the timebase has
    // changed
    dsoControl->setPretriggerPosition(
                settings->scope.trigger.position *
                settings->scope.horizontal.timebase * DIVS_TIME);

    dsoWidget->updateTimebase(settings->scope.horizontal.timebase);
}

/// \brief The oscilloscope changed the samplerate.
/// \param samplerate The new samplerate in samples per second.
void OpenHantekMainWindow::samplerateChanged(double samplerate) {
    if (!settings->scope.horizontal.samplerateSet &&
            settings->scope.horizontal.recordLength != UINT_MAX) {
        // The timebase was set, let's adapt the samplerate accordingly
        settings->scope.horizontal.samplerate = samplerate;
        horizontalDock->setSamplerate(samplerate);
    }

    dsoWidget->updateSamplerate(samplerate);
}

/// \brief Apply new record length to settings.
/// \param recordLength The selected record length in samples.
void OpenHantekMainWindow::recordLengthSelected(unsigned long recordLength) {
    dsoControl->setRecordLength(recordLength);
}

/// \brief Sets the samplerate of the oscilloscope.
void OpenHantekMainWindow::samplerateSelected() {
    dsoControl->setSamplerate(settings->scope.horizontal.samplerate);
}

/// \brief Sets the record time of the oscilloscope.
void OpenHantekMainWindow::timebaseSelected() {
    dsoControl->setRecordTime(settings->scope.horizontal.timebase *
                              DIVS_TIME);
    dsoWidget->updateTimebase(settings->scope.horizontal.timebase);
}

/// \brief Sets the offset of the oscilloscope for the given channel.
/// \param channel The channel that got a new offset.
void OpenHantekMainWindow::updateOffset(unsigned int channel) {
    if (channel >= settings->scope.physicalChannels)
        return;

    dsoControl->setOffset(
                channel,
                (settings->scope.voltage[channel].offset / DIVS_VOLTAGE) + 0.5);
}

/// \brief Sets the state of the given oscilloscope channel.
/// \param channel The channel whose state has changed.
void OpenHantekMainWindow::updateUsed(unsigned int channel) {
    if (channel >= (unsigned int)settings->scope.voltage.count())
        return;

    bool mathUsed =
            settings->scope.voltage[settings->scope.physicalChannels]
            .used |
            settings->scope.spectrum[settings->scope.physicalChannels]
            .used;

    // Normal channel, check if voltage/spectrum or math channel is used
    if (channel < settings->scope.physicalChannels)
        dsoControl->setChannelUsed(
                    channel, mathUsed | settings->scope.voltage[channel].used |
                    settings->scope.spectrum[channel].used);
    // Math channel, update all channels
    else if (channel == settings->scope.physicalChannels) {
        for (unsigned int channelCounter = 0;
             channelCounter < settings->scope.physicalChannels;
             ++channelCounter)
            dsoControl->setChannelUsed(
                        channelCounter,
                        mathUsed | settings->scope.voltage[channelCounter].used |
                        settings->scope.spectrum[channelCounter].used);
    }
}

/// \brief Sets the gain of the oscilloscope for the given channel.
/// \param channel The channel that got a new gain value.
void OpenHantekMainWindow::updateVoltageGain(unsigned int channel) {
    if (channel >= settings->scope.physicalChannels)
        return;

    dsoControl->setGain(
                channel, settings->scope.voltage[channel].gain * DIVS_VOLTAGE);
}

#ifdef DEBUG
/// \brief Send the command in the commandEdit to the oscilloscope.
void OpenHantekMainWindow::sendCommand() {
    int errorCode = dsoControl->stringCommand(commandEdit->text());

    commandEdit->hide();
    commandEdit->clear();

    if (errorCode < 0)
        statusBar()->showMessage(tr("Invalid command"), 3000);
}
#endif
