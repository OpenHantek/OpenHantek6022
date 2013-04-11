////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  openhantek.cpp
//
//  Copyright (C) 2010, 2011  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////


#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QStatusBar>


#include "openhantek.h"

#include "configdialog.h"
#include "dataanalyzer.h"
#include "dockwindows.h"
#include "dsocontrol.h"
#include "dsowidget.h"
#include "settings.h"
#include "hantek/control.h"


////////////////////////////////////////////////////////////////////////////////
// class OpenHantekMainWindow
/// \brief Initializes the gui elements of the main window.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
OpenHantekMainWindow::OpenHantekMainWindow(QWidget *parent, Qt::WindowFlags flags) : QMainWindow(parent, flags) {
	// Set application information
	QCoreApplication::setOrganizationName("paranoiacs.net");
	QCoreApplication::setOrganizationDomain("paranoiacs.net");
	QCoreApplication::setApplicationName("OpenHantek");	
	
	// Window title
	this->setWindowIcon(QIcon(":openhantek.png"));
	this->setWindowTitle(tr("OpenHantek"));
	
	// Create the controller for the oscilloscope, provides channel count for settings
	this->dsoControl = new Hantek::Control();
	
	// Application settings
	this->settings = new DsoSettings();
	this->settings->setChannelCount(this->dsoControl->getChannelCount());
	this->readSettings();
	
	// Create dock windows before the dso widget, they fix messed up settings
	this->createDockWindows();
	
	// The data analyzer
	this->dataAnalyzer = new DataAnalyzer(this->settings);
	
	// Central oszilloscope widget
	this->dsoWidget = new DsoWidget(this->settings, this->dataAnalyzer);
	this->setCentralWidget(this->dsoWidget);
	
	// Subroutines for window elements
	this->createActions();
	this->createToolBars();
	this->createMenus();
	this->createStatusBar();
	
	// Apply the settings after the gui is initialized
	this->applySettings();
	
	// Update stored window size and position
	this->settings->options.window.position = this->pos();
	this->settings->options.window.size = this->size();
	
	// Connect all signals
	this->connectSignals();
	
	// Set up the oscilloscope
	this->dsoControl->connectDevice();
	this->initializeDevice();
	this->dsoControl->startSampling();
}

/// \brief Cleans up the main window.
OpenHantekMainWindow::~OpenHantekMainWindow() {
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void OpenHantekMainWindow::closeEvent(QCloseEvent *event) {
	if(this->settings->options.alwaysSave)
		this->writeSettings();
	
	QMainWindow::closeEvent(event);
}

/// \brief Create the used actions.
void OpenHantekMainWindow::createActions() {
	this->openAction = new QAction(QIcon(":actions/open.png"), tr("&Open..."), this);
	this->openAction->setShortcut(tr("Ctrl+O"));
	this->openAction->setStatusTip(tr("Open saved settings"));
	connect(this->openAction, SIGNAL(triggered()), this, SLOT(open()));

	this->saveAction = new QAction(QIcon(":actions/save.png"), tr("&Save"), this);
	this->saveAction->setShortcut(tr("Ctrl+S"));
	this->saveAction->setStatusTip(tr("Save the current settings"));
	connect(this->saveAction, SIGNAL(triggered()), this, SLOT(save()));

	this->saveAsAction = new QAction(QIcon(":actions/save-as.png"), tr("Save &as..."), this);
	this->saveAsAction->setStatusTip(tr("Save the current settings to another file"));
	connect(this->saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

	this->printAction = new QAction(QIcon(":actions/print.png"), tr("&Print..."), this);
	this->printAction->setShortcut(tr("Ctrl+P"));
	this->printAction->setStatusTip(tr("Print the oscilloscope screen"));
	connect(this->printAction, SIGNAL(triggered()), this->dsoWidget, SLOT(print()));

	this->exportAsAction = new QAction(QIcon(":actions/export-as.png"), tr("&Export as..."), this);
	this->exportAsAction->setShortcut(tr("Ctrl+E"));
	this->exportAsAction->setStatusTip(tr("Export the oscilloscope data to a file"));
	connect(this->exportAsAction, SIGNAL(triggered()), this->dsoWidget, SLOT(exportAs()));

	this->exitAction = new QAction(tr("E&xit"), this);
	this->exitAction->setShortcut(tr("Ctrl+Q"));
	this->exitAction->setStatusTip(tr("Exit the application"));
	connect(this->exitAction, SIGNAL(triggered()), this, SLOT(close()));
	
	this->configAction = new QAction(tr("&Settings"), this);
	this->configAction->setShortcut(tr("Ctrl+S"));
	this->configAction->setStatusTip(tr("Configure the oscilloscope"));
	connect(this->configAction, SIGNAL(triggered()), this, SLOT(config()));
	
	this->startStopAction = new QAction(this);
	this->startStopAction->setShortcut(tr("Space"));
	this->stopped();

	this->digitalPhosphorAction = new QAction(QIcon(":actions/digitalphosphor.png"), tr("Digital &phosphor"), this);
	this->digitalPhosphorAction->setCheckable(true);
	this->digitalPhosphorAction->setChecked(this->settings->view.digitalPhosphor);
	this->digitalPhosphor(this->settings->view.digitalPhosphor);
	connect(this->digitalPhosphorAction, SIGNAL(toggled(bool)), this, SLOT(digitalPhosphor(bool)));
	
	this->zoomAction = new QAction(QIcon(":actions/zoom.png"), tr("&Zoom"), this);
	this->zoomAction->setCheckable(true);
	this->zoomAction->setChecked(this->settings->view.zoom);
	this->zoom(this->settings->view.zoom);
	connect(this->zoomAction, SIGNAL(toggled(bool)), this, SLOT(zoom(bool)));
	connect(this->zoomAction, SIGNAL(toggled(bool)), this->dsoWidget, SLOT(updateZoom(bool)));
	
	this->aboutAction = new QAction(tr("&About"), this);
	this->aboutAction->setStatusTip(tr("Show information about this program"));
	connect(this->aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	this->aboutQtAction = new QAction(tr("About &Qt"), this);
	this->aboutQtAction->setStatusTip(tr("Show the Qt library's About box"));
	connect(this->aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	
#ifdef DEBUG
	this->commandAction = new QAction(tr("Send command"), this);
	this->commandAction->setShortcut(tr("Shift+C"));
#endif
}

/// \brief Create the menus and menuitems.
void OpenHantekMainWindow::createMenus() {
	this->fileMenu = this->menuBar()->addMenu(tr("&File"));
	this->fileMenu->addAction(this->openAction);
	this->fileMenu->addAction(this->saveAction);
	this->fileMenu->addAction(this->saveAsAction);
	this->fileMenu->addSeparator();
	this->fileMenu->addAction(this->printAction);
	this->fileMenu->addAction(this->exportAsAction);
	this->fileMenu->addSeparator();
	this->fileMenu->addAction(this->exitAction);
	
	this->viewMenu = this->menuBar()->addMenu(tr("&View"));
	this->viewMenu->addAction(this->digitalPhosphorAction);
	this->viewMenu->addAction(this->zoomAction);
	this->viewMenu->addSeparator();
	this->dockMenu = this->viewMenu->addMenu(tr("&Docking windows"));
	this->dockMenu->addAction(this->horizontalDock->toggleViewAction());
	this->dockMenu->addAction(this->spectrumDock->toggleViewAction());
	this->dockMenu->addAction(this->triggerDock->toggleViewAction());
	this->dockMenu->addAction(this->voltageDock->toggleViewAction());
	this->toolbarMenu = this->viewMenu->addMenu(tr("&Toolbars"));
	this->toolbarMenu->addAction(this->fileToolBar->toggleViewAction());
	this->toolbarMenu->addAction(this->oscilloscopeToolBar->toggleViewAction());
	this->toolbarMenu->addAction(this->viewToolBar->toggleViewAction());

	this->oscilloscopeMenu = this->menuBar()->addMenu(tr("&Oscilloscope"));
	this->oscilloscopeMenu->addAction(this->configAction);
	this->oscilloscopeMenu->addSeparator();
	this->oscilloscopeMenu->addAction(this->startStopAction);
#ifdef DEBUG
	this->oscilloscopeMenu->addSeparator();
	this->oscilloscopeMenu->addAction(this->commandAction);
#endif

	this->menuBar()->addSeparator();

	this->helpMenu = this->menuBar()->addMenu(tr("&Help"));
	this->helpMenu->addAction(this->aboutAction);
	this->helpMenu->addAction(this->aboutQtAction);
}

/// \brief Create the toolbars and their buttons.
void OpenHantekMainWindow::createToolBars() {
	this->fileToolBar = new QToolBar(tr("File"));
	this->fileToolBar->setAllowedAreas(Qt::TopToolBarArea);
	this->fileToolBar->addAction(this->openAction);
	this->fileToolBar->addAction(this->saveAction);
	this->fileToolBar->addAction(this->saveAsAction);
	this->fileToolBar->addSeparator();
	this->fileToolBar->addAction(this->printAction);
	this->fileToolBar->addAction(this->exportAsAction);
	
	this->oscilloscopeToolBar = new QToolBar(tr("Oscilloscope"));
	this->oscilloscopeToolBar->addAction(this->startStopAction);
	
	this->viewToolBar = new QToolBar(tr("View"));
	this->viewToolBar->addAction(this->digitalPhosphorAction);
	this->viewToolBar->addAction(this->zoomAction);
}

/// \brief Create the status bar.
void OpenHantekMainWindow::createStatusBar() {
#ifdef DEBUG
	// Command field inside the status bar
	this->commandEdit = new QLineEdit();
	this->commandEdit->hide();

	this->statusBar()->addPermanentWidget(this->commandEdit, 1);
#endif
	
	this->statusBar()->showMessage(tr("Ready"));
	
#ifdef DEBUG
	connect(this->commandAction, SIGNAL(triggered()), this->commandEdit, SLOT(show()));
	connect(this->commandAction, SIGNAL(triggered()), this->commandEdit, SLOT(setFocus()));
	connect(this->commandEdit, SIGNAL(returnPressed()), this, SLOT(sendCommand()));
#endif
}

/// \brief Create all docking windows.
void OpenHantekMainWindow::createDockWindows()
{
	this->horizontalDock = new HorizontalDock(this->settings);
	this->triggerDock = new TriggerDock(this->settings, this->dsoControl->getSpecialTriggerSources());
	this->spectrumDock = new SpectrumDock(this->settings);
	this->voltageDock = new VoltageDock(this->settings);
}

/// \brief Connect general signals and device management signals.
void OpenHantekMainWindow::connectSignals() {
	// Connect general signals
	connect(this, SIGNAL(settingsChanged()), this, SLOT(applySettings()));
	//connect(this->dsoWidget, SIGNAL(stopped()), this, SLOT(stopped()));
	connect(this->dsoControl, SIGNAL(statusMessage(QString, int)), this->statusBar(), SLOT(showMessage(QString, int)));
	connect(this->dsoControl, SIGNAL(samplesAvailable(const std::vector<std::vector<double> > *, double, bool, QMutex *)), this->dataAnalyzer, SLOT(analyze(const std::vector<std::vector<double> > *, double, bool, QMutex *)));
	
	// Connect signals to DSO controller and widget
	connect(this->horizontalDock, SIGNAL(samplerateChanged(double)), this, SLOT(samplerateSelected()));
	connect(this->horizontalDock, SIGNAL(timebaseChanged(double)), this, SLOT(timebaseSelected()));
	connect(this->horizontalDock, SIGNAL(frequencybaseChanged(double)), this->dsoWidget, SLOT(updateFrequencybase(double)));
	connect(this->horizontalDock, SIGNAL(recordLengthChanged(unsigned long)), this, SLOT(recordLengthSelected(unsigned long)));
	//connect(this->horizontalDock, SIGNAL(formatChanged(HorizontalFormat)), this->dsoWidget, SLOT(horizontalFormatChanged(HorizontalFormat)));
	
	connect(this->triggerDock, SIGNAL(modeChanged(Dso::TriggerMode)), this->dsoControl, SLOT(setTriggerMode(Dso::TriggerMode)));
	connect(this->triggerDock, SIGNAL(modeChanged(Dso::TriggerMode)), this->dsoWidget, SLOT(updateTriggerMode()));
	connect(this->triggerDock, SIGNAL(sourceChanged(bool, unsigned int)), this->dsoControl, SLOT(setTriggerSource(bool, unsigned int)));
	connect(this->triggerDock, SIGNAL(sourceChanged(bool, unsigned int)), this->dsoWidget, SLOT(updateTriggerSource()));
	connect(this->triggerDock, SIGNAL(slopeChanged(Dso::Slope)), this->dsoControl, SLOT(setTriggerSlope(Dso::Slope)));
	connect(this->triggerDock, SIGNAL(slopeChanged(Dso::Slope)), this->dsoWidget, SLOT(updateTriggerSlope()));
	connect(this->dsoWidget, SIGNAL(triggerPositionChanged(double)), this->dsoControl, SLOT(setPretriggerPosition(double)));
	connect(this->dsoWidget, SIGNAL(triggerLevelChanged(unsigned int, double)), this->dsoControl, SLOT(setTriggerLevel(unsigned int, double)));
	
	connect(this->voltageDock, SIGNAL(usedChanged(unsigned int, bool)), this, SLOT(updateUsed(unsigned int)));
	connect(this->voltageDock, SIGNAL(usedChanged(unsigned int, bool)), this->dsoWidget, SLOT(updateVoltageUsed(unsigned int, bool)));
	connect(this->voltageDock, SIGNAL(couplingChanged(unsigned int, Dso::Coupling)), this->dsoControl, SLOT(setCoupling(unsigned int, Dso::Coupling)));
	connect(this->voltageDock, SIGNAL(couplingChanged(unsigned int, Dso::Coupling)), this->dsoWidget, SLOT(updateVoltageCoupling(unsigned int)));
	connect(this->voltageDock, SIGNAL(modeChanged(Dso::MathMode)), this->dsoWidget, SLOT(updateMathMode()));
	connect(this->voltageDock, SIGNAL(gainChanged(unsigned int, double)), this, SLOT(updateVoltageGain(unsigned int)));
	connect(this->voltageDock, SIGNAL(gainChanged(unsigned int, double)), this->dsoWidget, SLOT(updateVoltageGain(unsigned int)));
	connect(this->dsoWidget, SIGNAL(offsetChanged(unsigned int, double)), this, SLOT(updateOffset(unsigned int)));
	
	connect(this->spectrumDock, SIGNAL(usedChanged(unsigned int, bool)), this, SLOT(updateUsed(unsigned int)));
	connect(this->spectrumDock, SIGNAL(usedChanged(unsigned int, bool)), this->dsoWidget, SLOT(updateSpectrumUsed(unsigned int, bool)));
	connect(this->spectrumDock, SIGNAL(magnitudeChanged(unsigned int, double)), this->dsoWidget, SLOT(updateSpectrumMagnitude(unsigned int)));
	
	// Started/stopped signals from oscilloscope	
	connect(this->dsoControl, SIGNAL(samplingStarted()), this, SLOT(started()));
	connect(this->dsoControl, SIGNAL(samplingStopped()), this, SLOT(stopped()));
	
	//connect(this->dsoControl, SIGNAL(recordLengthChanged(unsigned long)), this, SLOT(recordLengthChanged()));
	connect(this->dsoControl, SIGNAL(recordTimeChanged(double)), this, SLOT(recordTimeChanged(double)));
	connect(this->dsoControl, SIGNAL(samplerateChanged(double)), this, SLOT(samplerateChanged(double)));
	
	connect(this->dsoControl, SIGNAL(availableRecordLengthsChanged(QList<unsigned int>)), this->horizontalDock, SLOT(availableRecordLengthsChanged(QList<unsigned int>)));
	connect(this->dsoControl, SIGNAL(samplerateLimitsChanged(double, double)), this->horizontalDock, SLOT(samplerateLimitsChanged(double, double)));
}

/// \brief Initialize the device with the current settings.
void OpenHantekMainWindow::initializeDevice() {
	for(unsigned int channel = 0; channel < this->settings->scope.physicalChannels; ++channel) {
		this->dsoControl->setCoupling(channel, (Dso::Coupling) this->settings->scope.voltage[channel].misc);
		this->updateVoltageGain(channel);
		this->updateOffset(channel);
		this->dsoControl->setTriggerLevel(channel, this->settings->scope.voltage[channel].trigger);
	}
	this->updateUsed(this->settings->scope.physicalChannels);
	if(this->settings->scope.horizontal.samplerateSet)
		this->samplerateSelected();
	else
		this->timebaseSelected();
	if(this->dsoControl->getAvailableRecordLengths()->isEmpty())
		this->dsoControl->setRecordLength(this->settings->scope.horizontal.recordLength);
	else {
		int index = this->dsoControl->getAvailableRecordLengths()->indexOf(this->settings->scope.horizontal.recordLength);
		this->dsoControl->setRecordLength(index < 0 ? 1 : index);
	}
	this->dsoControl->setTriggerMode(this->settings->scope.trigger.mode);
	this->dsoControl->setPretriggerPosition(this->settings->scope.trigger.position * this->settings->scope.horizontal.timebase * DIVS_TIME);
	this->dsoControl->setTriggerSlope(this->settings->scope.trigger.slope);
	this->dsoControl->setTriggerSource(this->settings->scope.trigger.special, this->settings->scope.trigger.source);
}

/// \brief Read the settings from an ini file.
/// \param fileName Optional filename to export the settings to a specific file.
/// \return 0 on success, negative on error.
int OpenHantekMainWindow::readSettings(const QString &fileName) {
	int status = this->settings->load(fileName);
	
	if(status == 0)
		emit(settingsChanged());
	
	return status;
}

/// \brief Save the settings to the harddisk.
/// \param fileName Optional filename to read the settings from an ini file.
/// \return 0 on success, negative on error.
int OpenHantekMainWindow::writeSettings(const QString &fileName) {
	this->updateSettings();
	
	return this->settings->save(fileName);
}

/// \brief Called everytime the window is moved.
/// \param event The move event, it isn't used here though.
void OpenHantekMainWindow::moveEvent(QMoveEvent *event) {
	Q_UNUSED(event);
	
	this->settings->options.window.position = this->pos();
}

/// \brief Called everytime the window is resized.
/// \param event The resize event, it isn't used here though.
void OpenHantekMainWindow::resizeEvent(QResizeEvent *event) {
	Q_UNUSED(event);
	
	this->settings->options.window.size = this->size();
}

/// \brief Open a configuration file.
/// \return 0 on success, 1 on user abort, negative on error.
int OpenHantekMainWindow::open() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr("Settings (*.ini)"));
	
	if(!fileName.isEmpty())
		return this->readSettings(fileName);
	else
		return 1;
}

/// \brief Save the current configuration to a file.
/// \return 0 on success, negative on error.
int OpenHantekMainWindow::save() {
	if (this->currentFile.isEmpty())
		return saveAs();
	else
		return this->writeSettings(this->currentFile);
}

/// \brief Save the configuration to another filename.
/// \return 0 on success, 1 on user abort, negative on error.
int OpenHantekMainWindow::saveAs() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings"), "", tr("Settings (*.ini)"));
	if (fileName.isEmpty())
		return 1;
	
	int status = this->writeSettings(fileName);
	
	if(status == 0)
		this->currentFile = fileName;
	
	return status;
}

/// \brief The oscilloscope started sampling.
void OpenHantekMainWindow::started() {
	this->startStopAction->setText(tr("&Stop"));
	this->startStopAction->setIcon(QIcon(":actions/stop.png"));
	this->startStopAction->setStatusTip(tr("Stop the oscilloscope"));
	
	disconnect(this->startStopAction, SIGNAL(triggered()), this->dsoControl, SLOT(startSampling()));
	connect(this->startStopAction, SIGNAL(triggered()), this->dsoControl, SLOT(stopSampling()));
}

/// \brief The oscilloscope stopped sampling.
void OpenHantekMainWindow::stopped() {
	this->startStopAction->setText(tr("&Start"));
	this->startStopAction->setIcon(QIcon(":actions/start.png"));
	this->startStopAction->setStatusTip(tr("Start the oscilloscope"));
	
	disconnect(this->startStopAction, SIGNAL(triggered()), this->dsoControl, SLOT(stopSampling()));
	connect(this->startStopAction, SIGNAL(triggered()), this->dsoControl, SLOT(startSampling()));
}

/// \brief Configure the oscilloscope.
void OpenHantekMainWindow::config() {
	this->updateSettings();
	
	DsoConfigDialog configDialog(this->settings, this);
	if(configDialog.exec() == QDialog::Accepted)
		this->settingsChanged();
}

/// \brief Enable/disable digital phosphor.
void OpenHantekMainWindow::digitalPhosphor(bool enabled) {
	this->settings->view.digitalPhosphor = enabled;
	
	if(this->settings->view.digitalPhosphor)
		this->digitalPhosphorAction->setStatusTip(tr("Disable fading of previous graphs"));
	else
		this->digitalPhosphorAction->setStatusTip(tr("Enable fading of previous graphs"));
}

/// \brief Show/hide the magnified scope.
void OpenHantekMainWindow::zoom(bool enabled) {
	this->settings->view.zoom = enabled;
	
	if(this->settings->view.zoom)
		this->zoomAction->setStatusTip(tr("Hide magnified scope"));
	else
		this->zoomAction->setStatusTip(tr("Show magnified scope"));
}

/// \brief Show the about dialog.
void OpenHantekMainWindow::about() {
	QMessageBox::about(this, tr("About OpenHantek %1").arg(VERSION), tr(
		"<p>This is a open source software for Hantek USB oscilloscopes.</p>"
		"<p>Copyright &copy; 2010, 2011 Oliver Haag &lt;oliver.haag@gmail.com&gt;</p>"
	));
}

/// \brief The settings have changed.
void OpenHantekMainWindow::applySettings() {
	// Main window
	if(!this->settings->options.window.position.isNull())
		this->move(this->settings->options.window.position);
	if(!this->settings->options.window.size.isNull())
		this->resize(this->settings->options.window.size);
	
	// Docking windows
	QList<QDockWidget *> docks;
	docks.append(this->horizontalDock);
	docks.append(this->triggerDock);
	docks.append(this->voltageDock);
	docks.append(this->spectrumDock);
	
	QList<DsoSettingsOptionsWindowPanel *> dockSettings;
	dockSettings.append(&(this->settings->options.window.dock.horizontal));
	dockSettings.append(&(this->settings->options.window.dock.trigger));
	dockSettings.append(&(this->settings->options.window.dock.voltage));
	dockSettings.append(&(this->settings->options.window.dock.spectrum));
	
	QList<int> dockedWindows[2]; // Docks docked on the sides of the main window
	
	for(int dockId = 0; dockId < docks.size(); ++dockId) {
		docks[dockId]->setVisible(dockSettings[dockId]->visible);
		if(!dockSettings[dockId]->position.isNull()) {
			if(dockSettings[dockId]->floating) {
				this->addDockWidget(Qt::RightDockWidgetArea, docks[dockId]);
				docks[dockId]->setFloating(dockSettings[dockId]->floating);
				docks[dockId]->move(dockSettings[dockId]->position);
			}
			else {
				// Check in which order the docking windows where placed
				int side = (dockSettings[dockId]->position.x() < this->settings->options.window.size.width() / 2) ? 0 : 1;
				int index = 0;
				while(index < dockedWindows[side].size() && dockSettings[dockedWindows[side][index]]->position.y() <= dockSettings[dockId]->position.y())
					++index;
				dockedWindows[side].insert(index, dockId);
			}
		}
		else {
			this->addDockWidget(Qt::RightDockWidgetArea, docks[dockId]);
		}
	}
	
	// Put the docked docking windows into the main window
	for(int position = 0; position < dockedWindows[0].size(); ++position)
		this->addDockWidget(Qt::LeftDockWidgetArea, docks[dockedWindows[0][position]]);
	for(int position = 0; position < dockedWindows[1].size(); ++position)
		this->addDockWidget(Qt::RightDockWidgetArea, docks[dockedWindows[1][position]]);
	
	// Toolbars
	QList<QToolBar *> toolbars;
	toolbars.append(this->fileToolBar);
	toolbars.append(this->oscilloscopeToolBar);
	toolbars.append(this->viewToolBar);
	
	QList<DsoSettingsOptionsWindowPanel *> toolbarSettings;
	toolbarSettings.append(&(this->settings->options.window.toolbar.file));
	toolbarSettings.append(&(this->settings->options.window.toolbar.oscilloscope));
	toolbarSettings.append(&(this->settings->options.window.toolbar.view));
	
	QList<int> dockedToolbars; // Docks docked on the sides of the main window
	
	for(int toolbarId = 0; toolbarId < toolbars.size(); ++toolbarId) {
		toolbars[toolbarId]->setVisible(toolbarSettings[toolbarId]->visible);
		//toolbars[toolbarId]->setFloating(toolbarSettings[toolbarId]->floating); // setFloating missing, a bug in Qt?
		if(!toolbarSettings[toolbarId]->position.isNull() && !toolbarSettings[toolbarId]->floating) {
			/*if(toolbarSettings[toolbarId]->floating) {
				toolbars[toolbarId]->move(toolbarSettings[toolbarId]->position);
			}
			else*/ {
				// Check in which order the toolbars where placed
				int index = 0;
				while(index < dockedToolbars.size() && toolbarSettings[dockedToolbars[index]]->position.x() <= toolbarSettings[toolbarId]->position.x())
					++index;
				dockedToolbars.insert(index, toolbarId);
			}
		}
		else {
			this->addToolBar(toolbars[toolbarId]);
		}
	}
	
	// Put the docked toolbars into the main window
	for(int position = 0; position < dockedToolbars.size(); ++position)
		this->addToolBar(toolbars[dockedToolbars[position]]);
}

/// \brief Update the window layout in the settings.
void OpenHantekMainWindow::updateSettings() {
	// Main window
	this->settings->options.window.position = this->pos();
	this->settings->options.window.size = this->size();
	
	// Docking windows
	QList<QDockWidget *> docks;
	docks.append(this->horizontalDock);
	docks.append(this->spectrumDock);
	docks.append(this->triggerDock);
	docks.append(this->voltageDock);
	
	QList<DsoSettingsOptionsWindowPanel *> dockSettings;
	dockSettings.append(&(this->settings->options.window.dock.horizontal));
	dockSettings.append(&(this->settings->options.window.dock.spectrum));
	dockSettings.append(&(this->settings->options.window.dock.trigger));
	dockSettings.append(&(this->settings->options.window.dock.voltage));
	
	for(int dockId = 0; dockId < docks.size(); ++dockId) {
		dockSettings[dockId]->floating = docks[dockId]->isFloating();
		dockSettings[dockId]->position = docks[dockId]->pos();
		dockSettings[dockId]->visible = docks[dockId]->isVisible();
	}
	
	// Toolbars
	QList<QToolBar *> toolbars;
	toolbars.append(this->fileToolBar);
	toolbars.append(this->oscilloscopeToolBar);
	toolbars.append(this->viewToolBar);
	
	QList<DsoSettingsOptionsWindowPanel *> toolbarSettings;
	toolbarSettings.append(&(this->settings->options.window.toolbar.file));
	toolbarSettings.append(&(this->settings->options.window.toolbar.oscilloscope));
	toolbarSettings.append(&(this->settings->options.window.toolbar.view));
	
	for(int toolbarId = 0; toolbarId < toolbars.size(); ++toolbarId) {
		toolbarSettings[toolbarId]->floating = toolbars[toolbarId]->isFloating();
		toolbarSettings[toolbarId]->position = toolbars[toolbarId]->pos();
		toolbarSettings[toolbarId]->visible = toolbars[toolbarId]->isVisible();
	}
}

/// \brief The oscilloscope changed the record time.
/// \param duration The new record time duration in seconds.
void OpenHantekMainWindow::recordTimeChanged(double duration) {
	if(this->settings->scope.horizontal.samplerateSet && this->settings->scope.horizontal.recordLength != UINT_MAX) {
		// The samplerate was set, let's adapt the timebase accordingly
		this->settings->scope.horizontal.timebase = duration / DIVS_TIME;
		this->horizontalDock->setTimebase(this->settings->scope.horizontal.timebase);
	}
	
	// The trigger position should be kept at the same place but the timebase has changed
	this->dsoControl->setPretriggerPosition(this->settings->scope.trigger.position * this->settings->scope.horizontal.timebase * DIVS_TIME);
	
	this->dsoWidget->updateTimebase(this->settings->scope.horizontal.timebase);
}

/// \brief The oscilloscope changed the samplerate.
/// \param samplerate The new samplerate in samples per second.
void OpenHantekMainWindow::samplerateChanged(double samplerate) {
	if(!this->settings->scope.horizontal.samplerateSet && this->settings->scope.horizontal.recordLength != UINT_MAX) {
		// The timebase was set, let's adapt the samplerate accordingly
		this->settings->scope.horizontal.samplerate = samplerate;
		this->horizontalDock->setSamplerate(samplerate);
	}
	
	this->dsoWidget->updateSamplerate(samplerate);
}

/// \brief Apply new record length to settings.
/// \param recordLength The selected record length in samples.
void OpenHantekMainWindow::recordLengthSelected(unsigned long recordLength) {
	this->dsoControl->setRecordLength(recordLength);
}

/// \brief Sets the samplerate of the oscilloscope.
void OpenHantekMainWindow::samplerateSelected() {
	this->dsoControl->setSamplerate(this->settings->scope.horizontal.samplerate);
}

/// \brief Sets the record time of the oscilloscope.
void OpenHantekMainWindow::timebaseSelected() {
	this->dsoControl->setRecordTime(this->settings->scope.horizontal.timebase * DIVS_TIME);
}

/// \brief Sets the offset of the oscilloscope for the given channel.
/// \param channel The channel that got a new offset.
void OpenHantekMainWindow::updateOffset(unsigned int channel) {
	if(channel >= this->settings->scope.physicalChannels)
		return;
	
	this->dsoControl->setOffset(channel, (this->settings->scope.voltage[channel].offset / DIVS_VOLTAGE) + 0.5);
}

/// \brief Sets the state of the given oscilloscope channel.
/// \param channel The channel whose state has changed.
void OpenHantekMainWindow::updateUsed(unsigned int channel) {
	if(channel >= (unsigned int) this->settings->scope.voltage.count())
		return;
	
	bool mathUsed = this->settings->scope.voltage[this->settings->scope.physicalChannels].used | this->settings->scope.spectrum[this->settings->scope.physicalChannels].used;
	
	// Normal channel, check if voltage/spectrum or math channel is used
	if(channel < this->settings->scope.physicalChannels)
		this->dsoControl->setChannelUsed(channel, mathUsed | this->settings->scope.voltage[channel].used | this->settings->scope.spectrum[channel].used);
	// Math channel, update all channels
	else if(channel == this->settings->scope.physicalChannels) {
		for(unsigned int channelCounter = 0; channelCounter < this->settings->scope.physicalChannels; ++channelCounter)
			this->dsoControl->setChannelUsed(channelCounter, mathUsed | this->settings->scope.voltage[channelCounter].used | this->settings->scope.spectrum[channelCounter].used);
	}
}

/// \brief Sets the gain of the oscilloscope for the given channel.
/// \param channel The channel that got a new gain value.
void OpenHantekMainWindow::updateVoltageGain(unsigned int channel) {
	if(channel >= this->settings->scope.physicalChannels)
		return;
	
	this->dsoControl->setGain(channel, this->settings->scope.voltage[channel].gain * DIVS_VOLTAGE);
}

#ifdef DEBUG
/// \brief Send the command in the commandEdit to the oscilloscope.
void OpenHantekMainWindow::sendCommand() {
	int errorCode = this->dsoControl->stringCommand(this->commandEdit->text());
	
	this->commandEdit->hide();
	this->commandEdit->clear();
	
	if(errorCode < 0)
		this->statusBar()->showMessage(tr("Invalid command"), 3000);
}
#endif
