////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  openhantek.cpp
//
//  Copyright (C) 2010  Oliver Haag
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
#include <QSettings>
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
	
	// Default window dimensions
	//this->move(152, 144);
	this->resize(720, 480);
	
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
	this->createMenus();
	this->createToolBars();
	this->createStatusBar();
	
	// Connect general signals
	connect(this, SIGNAL(settingsChanged()), this, SLOT(applySettings()));
	//connect(this->dsoWidget, SIGNAL(stopped()), this, SLOT(stopped()));
	connect(this->dsoControl, SIGNAL(statusMessage(QString, int)), this->statusBar(), SLOT(showMessage(QString, int)));
	connect(this->dsoControl, SIGNAL(samplesAvailable(const QList<double *> *, const QList<unsigned int> *, double, QMutex *)), this->dataAnalyzer, SLOT(analyze(const QList<double *> *, const QList<unsigned int> *, double, QMutex *)));
	
	// Connect signals to DSO controller and widget
	//connect(this->horizontalDock, SIGNAL(formatChanged(HorizontalFormat)), this->dsoWidget, SLOT(horizontalFormatChanged(HorizontalFormat)));
	connect(this->horizontalDock, SIGNAL(timebaseChanged(double)), this, SLOT(updateTimebase()));
	connect(this->horizontalDock, SIGNAL(timebaseChanged(double)), this->dsoWidget, SLOT(updateTimebase()));
	connect(this->horizontalDock, SIGNAL(frequencybaseChanged(double)), this->dsoWidget, SLOT(updateFrequencybase()));
	
	connect(this->triggerDock, SIGNAL(modeChanged(Dso::TriggerMode)), this->dsoControl, SLOT(setTriggerMode(Dso::TriggerMode)));
	connect(this->triggerDock, SIGNAL(modeChanged(Dso::TriggerMode)), this->dsoWidget, SLOT(updateTriggerMode()));
	connect(this->triggerDock, SIGNAL(sourceChanged(bool, unsigned int)), this->dsoControl, SLOT(setTriggerSource(bool, unsigned int)));
	connect(this->triggerDock, SIGNAL(sourceChanged(bool, unsigned int)), this->dsoWidget, SLOT(updateTriggerSource()));
	connect(this->triggerDock, SIGNAL(slopeChanged(Dso::Slope)), this->dsoControl, SLOT(setTriggerSlope(Dso::Slope)));
	connect(this->triggerDock, SIGNAL(slopeChanged(Dso::Slope)), this->dsoWidget, SLOT(updateTriggerSlope()));
	connect(this->dsoWidget, SIGNAL(triggerPositionChanged(double)), this->dsoControl, SLOT(setTriggerPosition(double)));
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
	
	// Set up the oscilloscope
	this->dsoControl->connectDevice();
	
	for(unsigned int channel = 0; channel < this->settings->scope.physicalChannels; channel++) {
		this->dsoControl->setCoupling(channel, (Dso::Coupling) this->settings->scope.voltage[channel].misc);
		this->updateVoltageGain(channel);
		this->updateOffset(channel);
		this->dsoControl->setTriggerLevel(channel, this->settings->scope.voltage[channel].trigger);
	}
	this->updateUsed(this->settings->scope.physicalChannels);
	this->dsoControl->setBufferSize(this->settings->scope.horizontal.samples);
	this->updateTimebase();
	this->dsoControl->setTriggerMode(this->settings->scope.trigger.mode);
	this->dsoControl->setTriggerPosition(this->settings->scope.trigger.position * this->settings->scope.horizontal.timebase * DIVS_TIME);
	this->dsoControl->setTriggerSlope(this->settings->scope.trigger.slope);
	this->dsoControl->setTriggerSource(this->settings->scope.trigger.special, this->settings->scope.trigger.source);
	
	this->dsoControl->startSampling();
}

/// \brief Cleans up the main window.
OpenHantekMainWindow::~OpenHantekMainWindow() {
}

/// \brief Save the settings before exiting.
/// \param event The close event that should be handled.
void OpenHantekMainWindow::closeEvent(QCloseEvent *event) {
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
	
	this->bufferSizeActionGroup = new QActionGroup(this);
	connect(this->bufferSizeActionGroup, SIGNAL(selected(QAction *)), this, SLOT(bufferSizeSelected(QAction *)));
	
	this->bufferSizeSmallAction = new QAction(tr("&Small"), this);
	this->bufferSizeSmallAction->setActionGroup(this->bufferSizeActionGroup);
	this->bufferSizeSmallAction->setCheckable(true);
	this->bufferSizeSmallAction->setChecked(this->settings->scope.horizontal.samples == Hantek::BUFFER_SMALL);
	this->bufferSizeSmallAction->setStatusTip(tr("10240 Samples"));

	this->bufferSizeLargeAction = new QAction(tr("&Large"), this);
	this->bufferSizeLargeAction->setActionGroup(this->bufferSizeActionGroup);
	this->bufferSizeLargeAction->setCheckable(true);
	this->bufferSizeLargeAction->setChecked(this->settings->scope.horizontal.samples == Hantek::BUFFER_LARGE);
	this->bufferSizeLargeAction->setStatusTip(tr("32768 Samples"));

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

	this->oscilloscopeMenu = this->menuBar()->addMenu(tr("&Oscilloscope"));
	this->oscilloscopeMenu->addAction(this->configAction);
	this->oscilloscopeMenu->addSeparator();
	this->oscilloscopeMenu->addAction(this->startStopAction);
#ifdef DEBUG
	this->oscilloscopeMenu->addAction(this->commandAction);
#endif
	this->oscilloscopeMenu->addSeparator();
	this->bufferSizeMenu = this->oscilloscopeMenu->addMenu(tr("&Buffer size"));
	this->bufferSizeMenu->addAction(this->bufferSizeSmallAction);
	this->bufferSizeMenu->addAction(this->bufferSizeLargeAction);

	this->menuBar()->addSeparator();

	this->helpMenu = this->menuBar()->addMenu(tr("&Help"));
	this->helpMenu->addAction(this->aboutAction);
	this->helpMenu->addAction(this->aboutQtAction);
}

/// \brief Create the toolbars and their buttons.
void OpenHantekMainWindow::createToolBars() {
	this->fileToolBar = this->addToolBar(tr("File"));
	this->fileToolBar->addAction(this->openAction);
	this->fileToolBar->addAction(this->saveAction);
	this->fileToolBar->addAction(this->saveAsAction);
	this->fileToolBar->addSeparator();
	this->fileToolBar->addAction(this->printAction);
	this->fileToolBar->addAction(this->exportAsAction);
	
	this->oscilloscopeToolBar = this->addToolBar(tr("Oscilloscope"));
	this->oscilloscopeToolBar->addAction(this->startStopAction);
	
	this->viewToolBar = this->addToolBar(tr("View"));
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
	
	this->addDockWidget(Qt::RightDockWidgetArea, this->horizontalDock);
	this->addDockWidget(Qt::RightDockWidgetArea, this->triggerDock);
	this->addDockWidget(Qt::RightDockWidgetArea, this->voltageDock);
	this->addDockWidget(Qt::RightDockWidgetArea, this->spectrumDock);
	
	//viewMenu->addAction(this->horizontalDock->toggleViewAction());
}

/// \brief Read the settings from the last session.
/// \param fileName Optional filename to export the settings to a specific file.
void OpenHantekMainWindow::readSettings(const QString &fileName) {
	// Use main configuration if the fileName wasn't set
	QSettings *settingsLoader;
	if(fileName.isEmpty())
		settingsLoader = new QSettings(this);
	else
		settingsLoader = new QSettings(fileName, QSettings::IniFormat, this);

	// Window size and position
	settingsLoader->beginGroup("window");
	if(settingsLoader->contains("pos"))
		this->move(settingsLoader->value("pos").toPoint());
	if(settingsLoader->contains("size"))
		this->resize(settingsLoader->value("size").toSize());
	settingsLoader->endGroup();
	
	// Oszilloskope settings
	settingsLoader->beginGroup("scope");
	// Horizontal axis
	settingsLoader->beginGroup("horizontal");
	if(settingsLoader->contains("format"))
		this->settings->scope.horizontal.format = (Dso::GraphFormat) settingsLoader->value("format").toInt();
	if(settingsLoader->contains("frequencybase"))
		this->settings->scope.horizontal.frequencybase = settingsLoader->value("frequencybase").toDouble();
	for(int marker = 0; marker < 2; marker++) {
		QString name;
		name = QString("marker%1").arg(marker);
		if(settingsLoader->contains(name))
			this->settings->scope.horizontal.marker[marker] = settingsLoader->value(name).toDouble();
	}
	if(settingsLoader->contains("timebase"))
		this->settings->scope.horizontal.timebase = settingsLoader->value("timebase").toDouble();
	settingsLoader->endGroup();
	// Trigger
	settingsLoader->beginGroup("trigger");
	if(settingsLoader->contains("filter"))
		this->settings->scope.trigger.filter = settingsLoader->value("filter").toBool();
	if(settingsLoader->contains("mode"))
		this->settings->scope.trigger.mode = (Dso::TriggerMode) settingsLoader->value("mode").toInt();
	if(settingsLoader->contains("position"))
		this->settings->scope.trigger.position = settingsLoader->value("position").toDouble();
	if(settingsLoader->contains("slope"))
		this->settings->scope.trigger.slope = (Dso::Slope) settingsLoader->value("slope").toInt();
	if(settingsLoader->contains("source"))
		this->settings->scope.trigger.source = settingsLoader->value("source").toInt();
	if(settingsLoader->contains("special"))
		this->settings->scope.trigger.special = settingsLoader->value("special").toInt();
	settingsLoader->endGroup();
	// Spectrum
	for(int channel = 0; channel < this->settings->scope.spectrum.count(); channel++) {
		settingsLoader->beginGroup(QString("spectrum%1").arg(channel));
		if(settingsLoader->contains("magnitude"))
			this->settings->scope.spectrum[channel].magnitude = settingsLoader->value("magnitude").toDouble();
		if(settingsLoader->contains("offset"))
			this->settings->scope.spectrum[channel].offset = settingsLoader->value("offset").toDouble();
		if(settingsLoader->contains("used"))
			this->settings->scope.spectrum[channel].used = settingsLoader->value("used").toBool();
		settingsLoader->endGroup();
	}
	// Vertical axis
	for(int channel = 0; channel < this->settings->scope.voltage.count(); channel++) {
		settingsLoader->beginGroup(QString("vertical%1").arg(channel));
		if(settingsLoader->contains("gain"))
			this->settings->scope.voltage[channel].gain = settingsLoader->value("gain").toDouble();
		if(settingsLoader->contains("misc"))
			this->settings->scope.voltage[channel].misc = settingsLoader->value("misc").toInt();
		if(settingsLoader->contains("offset"))
			this->settings->scope.voltage[channel].offset = settingsLoader->value("offset").toDouble();
		if(settingsLoader->contains("trigger"))
			this->settings->scope.voltage[channel].trigger = settingsLoader->value("trigger").toDouble();
		if(settingsLoader->contains("used"))
			this->settings->scope.voltage[channel].used = settingsLoader->value("used").toBool();
		settingsLoader->endGroup();
	}
	if(settingsLoader->contains("spectrumLimit"))
		this->settings->scope.spectrumLimit = settingsLoader->value("spectrumLimit").toDouble();
	if(settingsLoader->contains("spectrumReference"))
		this->settings->scope.spectrumReference = settingsLoader->value("spectrumReference").toDouble();
	if(settingsLoader->contains("spectrumWindow"))
		this->settings->scope.spectrumWindow = (Dso::WindowFunction) settingsLoader->value("spectrumWindow").toInt();
	settingsLoader->endGroup();
	
	// View
	settingsLoader->beginGroup("view");
	// Colors
	settingsLoader->beginGroup("color");
	DsoSettingsColorValues *colors;
	for(int mode = 0; mode < 2; mode++) {
		if(mode == 0) {
			colors = &this->settings->view.color.screen;
			settingsLoader->beginGroup("screen");
		}
		else {
			colors = &this->settings->view.color.print;
			settingsLoader->beginGroup("print");
		}
		
		if(settingsLoader->contains("axes"))
			colors->axes = settingsLoader->value("axes").value<QColor>();
		if(settingsLoader->contains("background"))
			colors->background = settingsLoader->value("background").value<QColor>();
		if(settingsLoader->contains("border"))
			colors->border = settingsLoader->value("border").value<QColor>();
		if(settingsLoader->contains("grid"))
			colors->grid = settingsLoader->value("grid").value<QColor>();
		if(settingsLoader->contains("markers"))
			colors->markers = settingsLoader->value("markers").value<QColor>();
		for(int channel = 0; channel < this->settings->scope.spectrum.count(); channel++) {
			QString key = QString("spectrum%1").arg(channel);
			if(settingsLoader->contains(key))
				colors->spectrum[channel] = settingsLoader->value(key).value<QColor>();
		}
		if(settingsLoader->contains("text"))
			colors->text = settingsLoader->value("text").value<QColor>();
		for(int channel = 0; channel < this->settings->scope.voltage.count(); channel++) {
			QString key = QString("voltage%1").arg(channel);
			if(settingsLoader->contains(key))
				colors->voltage[channel] = settingsLoader->value(key).value<QColor>();
		}
		settingsLoader->endGroup();
	}
	settingsLoader->endGroup();
	// Other view settings
	if(settingsLoader->contains("digitalPhosphor"))
		this->settings->view.digitalPhosphor = settingsLoader->value("digitalPhosphor").toBool();
	if(settingsLoader->contains("interpolation"))
		this->settings->view.interpolation = (Dso::InterpolationMode) settingsLoader->value("interpolation").toInt();
	if(settingsLoader->contains("screenColorImages"))
		this->settings->view.screenColorImages = (Dso::InterpolationMode) settingsLoader->value("screenColorImages").toBool();
	if(settingsLoader->contains("zoom"))
		this->settings->view.zoom = (Dso::InterpolationMode) settingsLoader->value("zoom").toBool();
	settingsLoader->endGroup();
	
	delete settingsLoader;

	emit(settingsChanged());
}

/// \brief Save the settings to the harddisk.
void OpenHantekMainWindow::writeSettings(const QString &fileName) {
	// Use main configuration and save everything if the fileName wasn't set
	QSettings *settingsSaver;
	bool complete = fileName.isEmpty();
	if(complete)
		settingsSaver = new QSettings(this);
	else
		settingsSaver = new QSettings(fileName, QSettings::IniFormat, this);

	if(complete) {
		// Window size and position
		settingsSaver->beginGroup("window");
		settingsSaver->setValue("pos", this->pos());
		settingsSaver->setValue("size", this->size());
		settingsSaver->endGroup();
	}
	// Oszilloskope settings
	settingsSaver->beginGroup("scope");
	// Horizontal axis
	settingsSaver->beginGroup("horizontal");
	settingsSaver->setValue("format", this->settings->scope.horizontal.format);
	settingsSaver->setValue("frequencybase", this->settings->scope.horizontal.frequencybase);
	for(int marker = 0; marker < 2; marker++)
		settingsSaver->setValue(QString("marker%1").arg(marker), this->settings->scope.horizontal.marker[marker]);
	settingsSaver->setValue("timebase", this->settings->scope.horizontal.timebase);
	settingsSaver->endGroup();
	// Trigger
	settingsSaver->beginGroup("trigger");
	settingsSaver->setValue("filter", this->settings->scope.trigger.filter);
	settingsSaver->setValue("mode", this->settings->scope.trigger.mode);
	settingsSaver->setValue("position", this->settings->scope.trigger.position);
	settingsSaver->setValue("slope", this->settings->scope.trigger.slope);
	settingsSaver->setValue("source", this->settings->scope.trigger.source);
	settingsSaver->endGroup();
	// Spectrum
	for(int channel = 0; channel < this->settings->scope.spectrum.count(); channel++) {
		settingsSaver->beginGroup(QString("spectrum%1").arg(channel));
		settingsSaver->setValue("magnitude", this->settings->scope.spectrum[channel].magnitude);
		settingsSaver->setValue("offset", this->settings->scope.spectrum[channel].offset);
		settingsSaver->setValue("used", this->settings->scope.spectrum[channel].used);
		settingsSaver->endGroup();
	}
	// Vertical axis
	for(int channel = 0; channel < this->settings->scope.voltage.count(); channel++) {
		settingsSaver->beginGroup(QString("vertical%1").arg(channel));
		settingsSaver->setValue("gain", this->settings->scope.voltage[channel].gain);
		settingsSaver->setValue("misc", this->settings->scope.voltage[channel].misc);
		settingsSaver->setValue("offset", this->settings->scope.voltage[channel].offset);
		settingsSaver->setValue("trigger", this->settings->scope.voltage[channel].trigger);
		settingsSaver->setValue("used", this->settings->scope.voltage[channel].used);
		settingsSaver->endGroup();
	}
	settingsSaver->setValue("spectrumLimit", this->settings->scope.spectrumLimit);
	settingsSaver->setValue("spectrumReference", this->settings->scope.spectrumReference);
	settingsSaver->setValue("spectrumWindow", this->settings->scope.spectrumWindow);
	settingsSaver->endGroup();
	
	// View
	settingsSaver->beginGroup("view");
	// Colors
	if(complete) {
		settingsSaver->beginGroup("color");
		DsoSettingsColorValues *colors;
		for(int mode = 0; mode < 2; mode++) {
			if(mode == 0) {
				colors = &this->settings->view.color.screen;
				settingsSaver->beginGroup("screen");
			}
			else {
				colors = &this->settings->view.color.print;
				settingsSaver->beginGroup("print");
			}
			
			settingsSaver->setValue("axes", colors->axes);
			settingsSaver->setValue("background", colors->background);
			settingsSaver->setValue("border", colors->border);
			settingsSaver->setValue("grid", colors->grid);
			settingsSaver->setValue("markers", colors->markers);
			for(int channel = 0; channel < this->settings->scope.spectrum.count(); channel++)
				settingsSaver->setValue(QString("spectrum%1").arg(channel), colors->spectrum[channel]);
			settingsSaver->setValue("text", colors->text);
			for(int channel = 0; channel < this->settings->scope.voltage.count(); channel++)
				settingsSaver->setValue(QString("voltage%1").arg(channel), colors->voltage[channel]);
			settingsSaver->endGroup();
		}
		settingsSaver->endGroup();
	}
	// Other view settings
	settingsSaver->setValue("digitalPhosphor", this->settings->view.digitalPhosphor);
	if(complete) {
		settingsSaver->setValue("interpolation", this->settings->view.interpolation);
		settingsSaver->setValue("screenColorImages", this->settings->view.screenColorImages);
	}
	settingsSaver->setValue("zoom", this->settings->view.zoom);
	settingsSaver->endGroup();
	
	delete settingsSaver;
}

/// \brief Open a existing file.
void OpenHantekMainWindow::open() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", "*.xml");
	if(!fileName.isEmpty())
		return; // TODO
}

/// \brief Save the file.
/// \return true if the file was saved, false if not.
bool OpenHantekMainWindow::save() {
	if (this->currentFile.isEmpty()) {
		return saveAs();
	} else {
		return false; /// \todo Saving of individual setting files
	}
}

/// \brief Save the mapping to another filename.
/// \return true if the file was saved, false if not.
bool OpenHantekMainWindow::saveAs() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings..."), "", "*.xml");
	if (fileName.isEmpty())
		return false;

	return false; /// \todo Saving of individual setting files
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
		"<p>Copyright &copy; 2010 Oliver Haag &lt;oliver.haag@gmail.com&gt;</p>"
	));
}

/// \brief The settings have changed.
void OpenHantekMainWindow::applySettings() {
}

/// \brief Apply new buffer size to settings.
/// \param action The selected buffer size menu item.
void OpenHantekMainWindow::bufferSizeSelected(QAction *action) {
	this->settings->scope.horizontal.samples = (action == this->bufferSizeSmallAction) ? Hantek::BUFFER_SMALL : Hantek::BUFFER_LARGE;
	this->dsoControl->setBufferSize(this->settings->scope.horizontal.samples);
}

/// \brief Sets the offset of the oscilloscope for the given channel.
/// \param channel The channel that got a new offset.
void OpenHantekMainWindow::updateOffset(unsigned int channel) {
	if(channel >= this->settings->scope.physicalChannels)
		return;
	
	this->dsoControl->setOffset(channel, (this->settings->scope.voltage[channel].offset / DIVS_VOLTAGE) + 0.5);
}

/// \brief Sets the samplerate of the oscilloscope.
void OpenHantekMainWindow::updateTimebase() {
	this->settings->scope.horizontal.samplerate = this->dsoControl->setSamplerate(1e3 / this->settings->scope.horizontal.timebase);
	this->dsoWidget->updateSamplerate();
	
	// The trigger position should be kept at the same place but the timebase has changed
	this->dsoControl->setTriggerPosition(this->settings->scope.trigger.position * this->settings->scope.horizontal.timebase * DIVS_TIME);
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
		for(unsigned int channelCounter = 0; channelCounter < this->settings->scope.physicalChannels; channelCounter++)
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
