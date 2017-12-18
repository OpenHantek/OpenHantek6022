// SPDX-License-Identifier: GPL-2.0+

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include "mainwindow.h"

#include "HorizontalDock.h"
#include "SpectrumDock.h"
#include "TriggerDock.h"
#include "VoltageDock.h"
#include "dockwindows.h"

#include "configdialog.h"
#include "dataanalyzer.h"
#include "dockwindows.h"
#include "dsowidget.h"
#include "hantek/hantekdsocontrol.h"
#include "hantekdsocontrol.h"
#include "usb/usbdevice.h"
#include "viewconstants.h"

////////////////////////////////////////////////////////////////////////////////
// class OpenHantekMainWindow
/// \brief Initializes the gui elements of the main window.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
OpenHantekMainWindow::OpenHantekMainWindow(HantekDsoControl *dsoControl, DataAnalyzer *dataAnalyzer,
                                           DsoSettings *settings)
    : dsoControl(dsoControl), dataAnalyzer(dataAnalyzer), settings(settings) {

    // Window title
    setWindowIcon(QIcon(":openhantek.png"));
    setWindowTitle(tr("OpenHantek - Device %1").arg(QString::fromStdString(dsoControl->getDevice()->getModel().name)));

// Create dock windows before the dso widget, they fix messed up settings
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
    createDockWindows();

    // Central oszilloscope widget
    dsoWidget = new DsoWidget(settings);
    connect(dataAnalyzer, &DataAnalyzer::analyzed,
            [this]() { dsoWidget->showNewData(this->dataAnalyzer->getNextResult()); });
    setCentralWidget(dsoWidget);

    // Subroutines for window elements
    createActions();
    createToolBars();
    createMenus();
    statusBar()->showMessage(tr("Ready"));

#ifdef DEBUG
    addManualCommandEdit();
#endif

    // Apply the settings after the gui is initialized
    applySettings();

    // Connect all signals
    connectSignals();

    // Set up the oscilloscope
    applySettingsToDevice();
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void OpenHantekMainWindow::closeEvent(QCloseEvent *event) {
    if (settings->options.alwaysSave) {
        saveWindowGeometry();
        settings->save();
    }

    QMainWindow::closeEvent(event);
}

/// \brief Create the used actions.
void OpenHantekMainWindow::createActions() {
    openAction = new QAction(QIcon(":actions/open.png"), tr("&Open..."), this);
    openAction->setShortcut(tr("Ctrl+O"));
    openAction->setStatusTip(tr("Open saved settings"));
    connect(openAction, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr("Settings (*.ini)"));
        if (!fileName.isEmpty()) {
            if (settings->setFilename(fileName)) {
                settings->load();
                emit(settingsChanged());
            }
        }
    });

    saveAction = new QAction(QIcon(":actions/save.png"), tr("&Save"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setStatusTip(tr("Save the current settings"));
    connect(saveAction, &QAction::triggered, [this]() {
        saveWindowGeometry();
        settings->save();
    });

    saveAsAction = new QAction(QIcon(":actions/save-as.png"), tr("Save &as..."), this);
    saveAsAction->setStatusTip(tr("Save the current settings to another file"));
    connect(saveAsAction, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings"), "", tr("Settings (*.ini)"));
        if (fileName.isEmpty()) return;
        saveWindowGeometry();
        settings->setFilename(fileName);
        settings->save();
    });

    printAction = new QAction(QIcon(":actions/print.png"), tr("&Print..."), this);
    printAction->setShortcut(tr("Ctrl+P"));
    printAction->setStatusTip(tr("Print the oscilloscope screen"));
    connect(printAction, &QAction::triggered, dsoWidget, &DsoWidget::print);

    exportAsAction = new QAction(QIcon(":actions/export-as.png"), tr("&Export as..."), this);
    exportAsAction->setShortcut(tr("Ctrl+E"));
    exportAsAction->setStatusTip(tr("Export the oscilloscope data to a file"));
    connect(exportAsAction, &QAction::triggered, dsoWidget, &DsoWidget::exportAs);

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    configAction = new QAction(tr("&Settings"), this);
    configAction->setShortcut(tr("Ctrl+S"));
    configAction->setStatusTip(tr("Configure the oscilloscope"));
    connect(configAction, &QAction::triggered, [this]() {
        saveWindowGeometry();

        DsoConfigDialog configDialog(settings, this);
        if (configDialog.exec() == QDialog::Accepted) settingsChanged();
    });

    startStopAction = new QAction(this);
    startStopAction->setShortcut(tr("Space"));
    stopped();

    digitalPhosphorAction = new QAction(QIcon(":actions/digitalphosphor.png"), tr("Digital &phosphor"), this);
    digitalPhosphorAction->setCheckable(true);
    digitalPhosphorAction->setChecked(settings->view.digitalPhosphor);
    digitalPhosphor(settings->view.digitalPhosphor);
    connect(digitalPhosphorAction, &QAction::toggled, this, &OpenHantekMainWindow::digitalPhosphor);

    zoomAction = new QAction(QIcon(":actions/zoom.png"), tr("&Zoom"), this);
    zoomAction->setCheckable(true);
    zoomAction->setChecked(settings->view.zoom);
    zoom(settings->view.zoom);
    connect(zoomAction, &QAction::toggled, this, &OpenHantekMainWindow::zoom);
    connect(zoomAction, &QAction::toggled, dsoWidget, &DsoWidget::updateZoom);

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show information about this program"));
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(
            this, tr("About OpenHantek %1").arg(VERSION),
            tr("<p>This is a open source software for Hantek USB oscilloscopes.</p>"
               "<p>Copyright &copy; 2010, 2011 Oliver Haag<br><a "
               "href='mailto:oliver.haag@gmail.com'>oliver.haag@gmail.com</a></p>"
               "<p>Copyright &copy; 2012-2017 OpenHantek community<br>"
               "<a href='https://github.com/OpenHantek/openhantek'>https://github.com/OpenHantek/openhantek</a></p>"));

    });
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

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

namespace {

QToolBar *CreateToolBar(const QString &title) {
    QToolBar *newObj = new QToolBar(title);
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

void OpenHantekMainWindow::addManualCommandEdit() {
    // Command field inside the status bar
    commandEdit = new QLineEdit();
    commandEdit->hide();

    commandAction = new QAction(tr("Send command"), this);
    commandAction->setShortcut(tr("Shift+C"));

    statusBar()->addPermanentWidget(commandEdit, 1);

    oscilloscopeMenu->addSeparator();
    oscilloscopeMenu->addAction(commandAction);

    connect(commandAction, &QAction::triggered, [this]() {
        commandEdit->show();
        commandEdit->setFocus();
    });
    connect(commandEdit, &QLineEdit::returnPressed, [this]() {
        int errorCode = dsoControl->stringCommand(commandEdit->text());

        commandEdit->hide();
        commandEdit->clear();

        if (errorCode < 0) statusBar()->showMessage(tr("Invalid command"), 3000);
    });
}

/// \brief Create all docking windows.
void OpenHantekMainWindow::createDockWindows() {
    registerDockMetaTypes();
    horizontalDock = new HorizontalDock(settings, this);
    triggerDock = new TriggerDock(settings, dsoControl->getSpecialTriggerSources(), this);
    spectrumDock = new SpectrumDock(settings, this);
    voltageDock = new VoltageDock(settings, this);
}

/// \brief Connect general signals and device management signals.
void OpenHantekMainWindow::connectSignals() {
    // Connect general signals
    connect(this, &OpenHantekMainWindow::settingsChanged, this, &OpenHantekMainWindow::applySettings);
    connect(dsoControl, &HantekDsoControl::statusMessage, statusBar(), &QStatusBar::showMessage);

    // Connect signals to DSO controller and widget
    connect(horizontalDock, &HorizontalDock::samplerateChanged, this, &OpenHantekMainWindow::samplerateSelected);
    connect(horizontalDock, &HorizontalDock::timebaseChanged, this, &OpenHantekMainWindow::timebaseSelected);
    connect(horizontalDock, &HorizontalDock::frequencybaseChanged, dsoWidget, &DsoWidget::updateFrequencybase);
    connect(horizontalDock, &HorizontalDock::recordLengthChanged, this, &OpenHantekMainWindow::recordLengthSelected);
    // connect(horizontalDock, SIGNAL(formatChanged(HorizontalFormat)),
    // dsoWidget, SLOT(horizontalFormatChanged(HorizontalFormat)));

    connect(triggerDock, &TriggerDock::modeChanged, dsoControl, &HantekDsoControl::setTriggerMode);
    connect(triggerDock, &TriggerDock::modeChanged, dsoWidget, &DsoWidget::updateTriggerMode);
    connect(triggerDock, &TriggerDock::sourceChanged, dsoControl, &HantekDsoControl::setTriggerSource);
    connect(triggerDock, &TriggerDock::sourceChanged, dsoWidget, &DsoWidget::updateTriggerSource);
    connect(triggerDock, &TriggerDock::slopeChanged, dsoControl, &HantekDsoControl::setTriggerSlope);
    connect(triggerDock, &TriggerDock::slopeChanged, dsoWidget, &DsoWidget::updateTriggerSlope);
    connect(dsoWidget, &DsoWidget::triggerPositionChanged, dsoControl, &HantekDsoControl::setPretriggerPosition);
    connect(dsoWidget, &DsoWidget::triggerLevelChanged, dsoControl, &HantekDsoControl::setTriggerLevel);

    connect(voltageDock, &VoltageDock::usedChanged, this, &OpenHantekMainWindow::updateUsed);
    connect(voltageDock, &VoltageDock::usedChanged, dsoWidget, &DsoWidget::updateVoltageUsed);
    connect(voltageDock, &VoltageDock::couplingChanged, dsoControl, &HantekDsoControl::setCoupling);
    connect(voltageDock, &VoltageDock::couplingChanged, dsoWidget, &DsoWidget::updateVoltageCoupling);
    connect(voltageDock, &VoltageDock::modeChanged, dsoWidget, &DsoWidget::updateMathMode);
    connect(voltageDock, &VoltageDock::gainChanged, this, &OpenHantekMainWindow::updateVoltageGain);
    connect(voltageDock, &VoltageDock::gainChanged, dsoWidget, &DsoWidget::updateVoltageGain);
    connect(dsoWidget, &DsoWidget::offsetChanged, this, &OpenHantekMainWindow::updateOffset);

    connect(spectrumDock, &SpectrumDock::usedChanged, this, &OpenHantekMainWindow::updateUsed);
    connect(spectrumDock, &SpectrumDock::usedChanged, dsoWidget, &DsoWidget::updateSpectrumUsed);
    connect(spectrumDock, &SpectrumDock::magnitudeChanged, dsoWidget, &DsoWidget::updateSpectrumMagnitude);

    // Started/stopped signals from oscilloscope
    connect(dsoControl, &HantekDsoControl::samplingStarted, this, &OpenHantekMainWindow::started);
    connect(dsoControl, &HantekDsoControl::samplingStopped, this, &OpenHantekMainWindow::stopped);

    connect(dsoControl, &HantekDsoControl::recordTimeChanged, this, &OpenHantekMainWindow::recordTimeChanged);
    connect(dsoControl, &HantekDsoControl::samplerateChanged, this, &OpenHantekMainWindow::samplerateChanged);

    connect(dsoControl, &HantekDsoControl::availableRecordLengthsChanged, horizontalDock,
            &HorizontalDock::availableRecordLengthsChanged);
    connect(dsoControl, &HantekDsoControl::samplerateLimitsChanged, horizontalDock,
            &HorizontalDock::samplerateLimitsChanged);
    connect(dsoControl, &HantekDsoControl::samplerateSet, horizontalDock, &HorizontalDock::samplerateSet);
}

/// \brief Initialize the device with the current settings.
void OpenHantekMainWindow::applySettingsToDevice() {
    for (unsigned int channel = 0; channel < settings->scope.physicalChannels; ++channel) {
        dsoControl->setCoupling(channel, (Dso::Coupling)settings->scope.voltage[channel].misc);
        updateVoltageGain(channel);
        updateOffset(channel);
        dsoControl->setTriggerLevel(channel, settings->scope.voltage[channel].trigger);
    }
    updateUsed(settings->scope.physicalChannels);
    if (settings->scope.horizontal.samplerateSet)
        samplerateSelected();
    else
        timebaseSelected();
    if (dsoControl->getAvailableRecordLengths()->isEmpty())
        dsoControl->setRecordLength(settings->scope.horizontal.recordLength);
    else {
        int index = dsoControl->getAvailableRecordLengths()->indexOf(settings->scope.horizontal.recordLength);
        dsoControl->setRecordLength(index < 0 ? 1 : index);
    }
    dsoControl->setTriggerMode(settings->scope.trigger.mode);
    dsoControl->setPretriggerPosition(settings->scope.trigger.position * settings->scope.horizontal.timebase *
                                      DIVS_TIME);
    dsoControl->setTriggerSlope(settings->scope.trigger.slope);
    dsoControl->setTriggerSource(settings->scope.trigger.special, settings->scope.trigger.source);
}

/// \brief The oscilloscope started sampling.
void OpenHantekMainWindow::started() {
    startStopAction->setText(tr("&Stop"));
    startStopAction->setIcon(QIcon(":actions/stop.png"));
    startStopAction->setStatusTip(tr("Stop the oscilloscope"));

    disconnect(startStopAction, &QAction::triggered, dsoControl, &HantekDsoControl::startSampling);
    connect(startStopAction, &QAction::triggered, dsoControl, &HantekDsoControl::stopSampling);
}

/// \brief The oscilloscope stopped sampling.
void OpenHantekMainWindow::stopped() {
    startStopAction->setText(tr("&Start"));
    startStopAction->setIcon(QIcon(":actions/start.png"));
    startStopAction->setStatusTip(tr("Start the oscilloscope"));

    disconnect(startStopAction, &QAction::triggered, dsoControl, &HantekDsoControl::stopSampling);
    connect(startStopAction, &QAction::triggered, dsoControl, &HantekDsoControl::startSampling);
}

/// \brief Enable/disable digital phosphor.
void OpenHantekMainWindow::digitalPhosphor(bool enabled) {
    settings->view.digitalPhosphor = enabled;

    if (settings->view.digitalPhosphor)
        digitalPhosphorAction->setStatusTip(tr("Disable fading of previous graphs"));
    else
        digitalPhosphorAction->setStatusTip(tr("Enable fading of previous graphs"));
}

/// \brief Show/hide the magnified scope.
void OpenHantekMainWindow::zoom(bool enabled) {
    settings->view.zoom = enabled;

    if (settings->view.zoom)
        zoomAction->setStatusTip(tr("Hide magnified scope"));
    else
        zoomAction->setStatusTip(tr("Show magnified scope"));
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
void OpenHantekMainWindow::saveWindowGeometry() {
    // Main window
    settings->mainWindowGeometry = saveGeometry();
    settings->mainWindowState = saveState();
}

/// \brief The oscilloscope changed the record time.
/// \param duration The new record time duration in seconds.
void OpenHantekMainWindow::recordTimeChanged(double duration) {
    if (settings->scope.horizontal.samplerateSet && settings->scope.horizontal.recordLength != UINT_MAX) {
        // The samplerate was set, let's adapt the timebase accordingly
        settings->scope.horizontal.timebase = horizontalDock->setTimebase(duration / DIVS_TIME);
    }

    // The trigger position should be kept at the same place but the timebase has
    // changed
    dsoControl->setPretriggerPosition(settings->scope.trigger.position * settings->scope.horizontal.timebase *
                                      DIVS_TIME);

    dsoWidget->updateTimebase(settings->scope.horizontal.timebase);
}

/// \brief The oscilloscope changed the samplerate.
/// \param samplerate The new samplerate in samples per second.
void OpenHantekMainWindow::samplerateChanged(double samplerate) {
    if (!settings->scope.horizontal.samplerateSet && settings->scope.horizontal.recordLength != UINT_MAX) {
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
void OpenHantekMainWindow::samplerateSelected() { dsoControl->setSamplerate(settings->scope.horizontal.samplerate); }

/// \brief Sets the record time of the oscilloscope.
void OpenHantekMainWindow::timebaseSelected() {
    dsoControl->setRecordTime(settings->scope.horizontal.timebase * DIVS_TIME);
    dsoWidget->updateTimebase(settings->scope.horizontal.timebase);
}

/// \brief Sets the offset of the oscilloscope for the given channel.
/// \param channel The channel that got a new offset.
void OpenHantekMainWindow::updateOffset(unsigned int channel) {
    if (channel >= settings->scope.physicalChannels) return;

    dsoControl->setOffset(channel, (settings->scope.voltage[channel].offset / DIVS_VOLTAGE) + 0.5);
}

/// \brief Sets the state of the given oscilloscope channel.
/// \param channel The channel whose state has changed.
void OpenHantekMainWindow::updateUsed(unsigned int channel) {
    if (channel >= (unsigned int)settings->scope.voltage.count()) return;

    bool mathUsed = settings->scope.voltage[settings->scope.physicalChannels].used |
                    settings->scope.spectrum[settings->scope.physicalChannels].used;

    // Normal channel, check if voltage/spectrum or math channel is used
    if (channel < settings->scope.physicalChannels)
        dsoControl->setChannelUsed(
            channel, mathUsed | settings->scope.voltage[channel].used | settings->scope.spectrum[channel].used);
    // Math channel, update all channels
    else if (channel == settings->scope.physicalChannels) {
        for (unsigned int channelCounter = 0; channelCounter < settings->scope.physicalChannels; ++channelCounter)
            dsoControl->setChannelUsed(channelCounter,
                                       mathUsed | settings->scope.voltage[channelCounter].used |
                                           settings->scope.spectrum[channelCounter].used);
    }
}

/// \brief Sets the gain of the oscilloscope for the given channel.
/// \param channel The channel that got a new gain value.
void OpenHantekMainWindow::updateVoltageGain(unsigned int channel) {
    if (channel >= settings->scope.physicalChannels) return;

    dsoControl->setGain(channel, settings->scope.voltage[channel].gain * DIVS_VOLTAGE);
}
