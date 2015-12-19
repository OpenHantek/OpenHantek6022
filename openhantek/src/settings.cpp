////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  settings.cpp
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


#include <QColor>
#include <QSettings>


#include "settings.h"

#include "dso.h"
#include "dsowidget.h"


////////////////////////////////////////////////////////////////////////////////
// class DsoSettings
/// \brief Sets the values to their defaults.
DsoSettings::DsoSettings(QWidget *parent) : QObject(parent) {
	// Options
	this->options.alwaysSave = true;
	this->options.imageSize = QSize(640, 480);
	// Main window
	this->options.window.position = QPoint();
	this->options.window.size = QSize(800, 600);
	// Docking windows and toolbars
	QList<DsoSettingsOptionsWindowPanel *> panels;
	panels.append(&(this->options.window.dock.horizontal));
	panels.append(&(this->options.window.dock.spectrum));
	panels.append(&(this->options.window.dock.trigger));
	panels.append(&(this->options.window.dock.voltage));
	panels.append(&(this->options.window.toolbar.file));
	panels.append(&(this->options.window.toolbar.oscilloscope));
	panels.append(&(this->options.window.toolbar.view));
	for(int panelId = 0; panelId < panels.size(); ++panelId) {
		panels[panelId]->floating = false;
		panels[panelId]->position = QPoint();
		panels[panelId]->visible = true;
	}
	
	// Oscilloscope settings
	// Horizontal axis
	this->scope.horizontal.format = Dso::GRAPHFORMAT_TY;
	this->scope.horizontal.frequencybase = 1e3;
	this->scope.horizontal.marker[0] = -1.0;
	this->scope.horizontal.marker[1] = 1.0;
	this->scope.horizontal.timebase = 1e-3;
	this->scope.horizontal.recordLength = 0;
	this->scope.horizontal.samplerate = 1e6;
	this->scope.horizontal.samplerateSet = false;
	// Trigger
	this->scope.trigger.filter = true;
	this->scope.trigger.mode = Dso::TRIGGERMODE_NORMAL;
	this->scope.trigger.position = 0.0;
	this->scope.trigger.slope = Dso::SLOPE_POSITIVE;
	this->scope.trigger.source = 0;
	this->scope.trigger.special = false;
	// General
	this->scope.physicalChannels = 0;
	this->scope.spectrumLimit = -20.0;
	this->scope.spectrumReference = 0.0;
	this->scope.spectrumWindow = Dso::WINDOW_HANN;
	
	
	// View
	// Colors
	// Screen
	this->view.color.screen.axes = QColor(0xff, 0xff, 0xff, 0x7f);
	this->view.color.screen.background = QColor(0x00, 0x00, 0x00, 0xff);
	this->view.color.screen.border = QColor(0xff, 0xff, 0xff, 0xff);
	this->view.color.screen.grid = QColor(0xff, 0xff, 0xff, 0x3f);
	this->view.color.screen.markers = QColor(0xff, 0xff, 0xff, 0xbf);
	this->view.color.screen.text = QColor(0xff, 0xff, 0xff, 0xff);
	// Print
	this->view.color.print.axes = QColor(0x00, 0x00, 0x00, 0xbf);
	this->view.color.print.background = QColor(0x00, 0x00, 0x00, 0x00);
	this->view.color.print.border = QColor(0x00, 0x00, 0x00, 0xff);
	this->view.color.print.grid = QColor(0x00, 0x00, 0x00, 0x7f);
	this->view.color.print.markers = QColor(0x00, 0x00, 0x00, 0xef);
	this->view.color.print.text = QColor(0x00, 0x00, 0x00, 0xff);
	// Other view settings
	this->view.antialiasing = true;
	this->view.digitalPhosphor = false;
	this->view.digitalPhosphorDepth = 8;
	this->view.interpolation = Dso::INTERPOLATION_LINEAR;
	this->view.screenColorImages = false;
	this->view.zoom = false;
}

/// \brief Cleans up.
DsoSettings::~DsoSettings() {
}

/// \brief Set the number of channels.
/// \param channels The new channel count, that will be applied to lists.
void DsoSettings::setChannelCount(unsigned int channels) {
	this->scope.physicalChannels = channels;
	// Always put the math channel at the end of the list
	
	// Remove list items for removed channels
	for(int channel = this->scope.spectrum.count() - 2; channel >= (int) channels; channel--)
		this->scope.spectrum.removeAt(channel);
	for(int channel = this->scope.voltage.count() - 2; channel >= (int) channels; channel--)
		this->scope.voltage.removeAt(channel);
	for(int channel = this->scope.spectrum.count() - 2; channel >= (int) channels; channel--)
		this->scope.spectrum.removeAt(channel);
	for(int channel = this->scope.spectrum.count() - 2; channel >= (int) channels; channel--)
		this->scope.spectrum.removeAt(channel);
	
	// Add new channels to the list
	for(int channel = 0; channel < (int) channels; ++channel) {
		// Oscilloscope settings
		// Spectrum
		if(this->scope.spectrum.count() <= channel + 1) {
			DsoSettingsScopeSpectrum newSpectrum;
			newSpectrum.magnitude = 20.0;
			newSpectrum.name = QApplication::tr("SP%1").arg(channel + 1);
			newSpectrum.offset = 0.0;
			newSpectrum.used = false;
			this->scope.spectrum.insert(channel, newSpectrum);
		}
		// Voltage
		if(this->scope.voltage.count() <= channel + 1) {
			DsoSettingsScopeVoltage newVoltage;
			newVoltage.gain = 1.0;
			newVoltage.misc = Dso::COUPLING_DC;
			newVoltage.name = QApplication::tr("CH%1").arg(channel + 1);
			newVoltage.offset = 0.0;
			newVoltage.trigger = 0.0;
			newVoltage.used = false;
			this->scope.voltage.insert(channel, newVoltage);
		}
		
		// View
		// Colors
		// Screen
		if(this->view.color.screen.voltage.count() <= channel + 1)
			this->view.color.screen.voltage.insert(channel, QColor::fromHsv(channel * 60, 0xff, 0xff));
		if(this->view.color.screen.spectrum.count() <= channel + 1)
			this->view.color.screen.spectrum.insert(channel, this->view.color.screen.voltage[channel].lighter());
		// Print
		if(this->view.color.print.voltage.count() <= channel + 1)
			this->view.color.print.voltage.insert(channel, this->view.color.screen.voltage[channel].darker(120));
		if(this->view.color.print.spectrum.count() <= channel + 1)
			this->view.color.print.spectrum.insert(channel, this->view.color.screen.voltage[channel].darker());
	}
	
	// Check if the math channel is missing
	if(this->scope.spectrum.count() <= (int) channels) {
		DsoSettingsScopeSpectrum newSpectrum;
		newSpectrum.magnitude = 20.0;
		newSpectrum.name = QApplication::tr("SPM");
		newSpectrum.offset = 0.0;
		newSpectrum.used = false;
		this->scope.spectrum.append(newSpectrum);
	}
	if(this->scope.voltage.count() <= (int) channels) {
		DsoSettingsScopeVoltage newVoltage;
		newVoltage.gain = 1.0;
		newVoltage.misc = Dso::MATHMODE_1ADD2;
		newVoltage.name = QApplication::tr("MATH");
		newVoltage.offset = 0.0;
		newVoltage.trigger = 0.0;
		newVoltage.used = false;
		this->scope.voltage.append(newVoltage);
	}
	if(this->view.color.screen.voltage.count() <= (int) channels)
		this->view.color.screen.voltage.append(QColor(0x7f, 0x7f, 0x7f, 0xff));
	if(this->view.color.screen.spectrum.count() <= (int) channels)
		this->view.color.screen.spectrum.append(this->view.color.screen.voltage[channels].lighter());
	if(this->view.color.print.voltage.count() <= (int) channels)
		this->view.color.print.voltage.append(this->view.color.screen.voltage[channels]);
	if(this->view.color.print.spectrum.count() <= (int) channels)
		this->view.color.print.spectrum.append(this->view.color.print.voltage[channels].darker());
}

/// \brief Read the settings from the last session or another file.
/// \param fileName Optional filename to load the settings from an ini file.
/// \return 0 on success, negative on error.
int DsoSettings::load(const QString &fileName) {
	// Use main configuration if the fileName wasn't set
	QSettings *settingsLoader;
	if(fileName.isEmpty())
		settingsLoader = new QSettings(this);
	else {
		settingsLoader = new QSettings(fileName, QSettings::IniFormat, this);
	}
	if(settingsLoader->status() != QSettings::NoError)
		return -settingsLoader->status();

	// Main window layout and other general options
	settingsLoader->beginGroup("options");
	settingsLoader->beginGroup("window");
	// Docking windows and toolbars
	settingsLoader->beginGroup("docks");
	QList<DsoSettingsOptionsWindowPanel *> docks;
	docks.append(&(this->options.window.dock.horizontal));
	docks.append(&(this->options.window.dock.spectrum));
	docks.append(&(this->options.window.dock.trigger));
	docks.append(&(this->options.window.dock.voltage));
	QStringList dockNames;
	dockNames << "horizontal" << "spectrum" << "trigger" << "voltage";
	for(int dockId = 0; dockId < docks.size(); ++dockId) {
		settingsLoader->beginGroup(dockNames[dockId]);
		if(settingsLoader->contains("floating"))
			docks[dockId]->floating = settingsLoader->value("floating").toBool();
		if(settingsLoader->contains("position"))
			docks[dockId]->position = settingsLoader->value("position").toPoint();
		if(settingsLoader->contains("visible"))
			docks[dockId]->visible = settingsLoader->value("visible").toBool();
		settingsLoader->endGroup();
	}
	settingsLoader->endGroup();
	settingsLoader->beginGroup("toolbars");
	QList<DsoSettingsOptionsWindowPanel *> toolbars;
	toolbars.append(&(this->options.window.toolbar.file));
	toolbars.append(&(this->options.window.toolbar.oscilloscope));
	toolbars.append(&(this->options.window.toolbar.view));
	QStringList toolbarNames;
	toolbarNames << "file" << "oscilloscope" << "view";
	for(int toolbarId = 0; toolbarId < toolbars.size(); ++toolbarId) {
		settingsLoader->beginGroup(toolbarNames[toolbarId]);
		if(settingsLoader->contains("floating"))
			toolbars[toolbarId]->floating = settingsLoader->value("floating").toBool();
		if(settingsLoader->contains("position"))
			toolbars[toolbarId]->position = settingsLoader->value("position").toPoint();
		if(settingsLoader->contains("visible"))
			toolbars[toolbarId]->visible = settingsLoader->value("visible").toBool();
		settingsLoader->endGroup();
	}
	settingsLoader->endGroup();
	// Main window
	if(settingsLoader->contains("pos"))
		this->options.window.position = settingsLoader->value("pos").toPoint();
	if(settingsLoader->contains("size"))
		this->options.window.size = settingsLoader->value("size").toSize();
	settingsLoader->endGroup();
	// General options
	if(settingsLoader->contains("alwaysSave"))
		this->options.alwaysSave = settingsLoader->value("alwaysSave").toBool();
	if(settingsLoader->contains("imageSize"))
		this->options.imageSize = settingsLoader->value("imageSize").toSize();
	settingsLoader->endGroup();
	
	// Oszilloskope settings
	settingsLoader->beginGroup("scope");
	// Horizontal axis
	settingsLoader->beginGroup("horizontal");
	if(settingsLoader->contains("format"))
		this->scope.horizontal.format = (Dso::GraphFormat) settingsLoader->value("format").toInt();
	if(settingsLoader->contains("frequencybase"))
		this->scope.horizontal.frequencybase = settingsLoader->value("frequencybase").toDouble();
	for(int marker = 0; marker < 2; ++marker) {
		QString name;
		name = QString("marker%1").arg(marker);
		if(settingsLoader->contains(name))
			this->scope.horizontal.marker[marker] = settingsLoader->value(name).toDouble();
	}
	if(settingsLoader->contains("timebase"))
		this->scope.horizontal.timebase = settingsLoader->value("timebase").toDouble();
	if(settingsLoader->contains("recordLength"))
		this->scope.horizontal.recordLength = settingsLoader->value("recordLength").toUInt();
	if(settingsLoader->contains("samplerate"))
		this->scope.horizontal.samplerate = settingsLoader->value("samplerate").toDouble();
	if(settingsLoader->contains("samplerateSet"))
		this->scope.horizontal.samplerateSet = settingsLoader->value("samplerateSet").toBool();
	settingsLoader->endGroup();
	// Trigger
	settingsLoader->beginGroup("trigger");
	if(settingsLoader->contains("filter"))
		this->scope.trigger.filter = settingsLoader->value("filter").toBool();
	if(settingsLoader->contains("mode"))
		this->scope.trigger.mode = (Dso::TriggerMode) settingsLoader->value("mode").toInt();
	if(settingsLoader->contains("position"))
		this->scope.trigger.position = settingsLoader->value("position").toDouble();
	if(settingsLoader->contains("slope"))
		this->scope.trigger.slope = (Dso::Slope) settingsLoader->value("slope").toInt();
	if(settingsLoader->contains("source"))
		this->scope.trigger.source = settingsLoader->value("source").toInt();
	if(settingsLoader->contains("special"))
		this->scope.trigger.special = settingsLoader->value("special").toInt();
	settingsLoader->endGroup();
	// Spectrum
	for(int channel = 0; channel < this->scope.spectrum.count(); ++channel) {
		settingsLoader->beginGroup(QString("spectrum%1").arg(channel));
		if(settingsLoader->contains("magnitude"))
			this->scope.spectrum[channel].magnitude = settingsLoader->value("magnitude").toDouble();
		if(settingsLoader->contains("offset"))
			this->scope.spectrum[channel].offset = settingsLoader->value("offset").toDouble();
		if(settingsLoader->contains("used"))
			this->scope.spectrum[channel].used = settingsLoader->value("used").toBool();
		settingsLoader->endGroup();
	}
	// Vertical axis
	for(int channel = 0; channel < this->scope.voltage.count(); ++channel) {
		settingsLoader->beginGroup(QString("vertical%1").arg(channel));
		if(settingsLoader->contains("gain"))
			this->scope.voltage[channel].gain = settingsLoader->value("gain").toDouble();
		if(settingsLoader->contains("misc"))
			this->scope.voltage[channel].misc = settingsLoader->value("misc").toInt();
		if(settingsLoader->contains("offset"))
			this->scope.voltage[channel].offset = settingsLoader->value("offset").toDouble();
		if(settingsLoader->contains("trigger"))
			this->scope.voltage[channel].trigger = settingsLoader->value("trigger").toDouble();
		if(settingsLoader->contains("used"))
			this->scope.voltage[channel].used = settingsLoader->value("used").toBool();
		settingsLoader->endGroup();
	}
	if(settingsLoader->contains("spectrumLimit"))
		this->scope.spectrumLimit = settingsLoader->value("spectrumLimit").toDouble();
	if(settingsLoader->contains("spectrumReference"))
		this->scope.spectrumReference = settingsLoader->value("spectrumReference").toDouble();
	if(settingsLoader->contains("spectrumWindow"))
		this->scope.spectrumWindow = (Dso::WindowFunction) settingsLoader->value("spectrumWindow").toInt();
	settingsLoader->endGroup();
	
	// View
	settingsLoader->beginGroup("view");
	// Colors
	settingsLoader->beginGroup("color");
	DsoSettingsColorValues *colors;
	for(int mode = 0; mode < 2; ++mode) {
		if(mode == 0) {
			colors = &this->view.color.screen;
			settingsLoader->beginGroup("screen");
		}
		else {
			colors = &this->view.color.print;
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
		for(int channel = 0; channel < this->scope.spectrum.count(); ++channel) {
			QString key = QString("spectrum%1").arg(channel);
			if(settingsLoader->contains(key))
				colors->spectrum[channel] = settingsLoader->value(key).value<QColor>();
		}
		if(settingsLoader->contains("text"))
			colors->text = settingsLoader->value("text").value<QColor>();
		for(int channel = 0; channel < this->scope.voltage.count(); ++channel) {
			QString key = QString("voltage%1").arg(channel);
			if(settingsLoader->contains(key))
				colors->voltage[channel] = settingsLoader->value(key).value<QColor>();
		}
		settingsLoader->endGroup();
	}
	settingsLoader->endGroup();
	// Other view settings
	if(settingsLoader->contains("digitalPhosphor"))
		this->view.digitalPhosphor = settingsLoader->value("digitalPhosphor").toBool();
	if(settingsLoader->contains("interpolation"))
		this->view.interpolation = (Dso::InterpolationMode) settingsLoader->value("interpolation").toInt();
	if(settingsLoader->contains("screenColorImages"))
		this->view.screenColorImages = (Dso::InterpolationMode) settingsLoader->value("screenColorImages").toBool();
	if(settingsLoader->contains("zoom"))
		this->view.zoom = (Dso::InterpolationMode) settingsLoader->value("zoom").toBool();
	settingsLoader->endGroup();
	
	delete settingsLoader;
	
	return 0;
}

/// \brief Save the settings to the harddisk.
/// \param fileName Optional filename to read the settings from an ini file.
/// \return 0 on success, negative on error.
int DsoSettings::save(const QString &fileName) {
	// Use main configuration and save everything if the fileName wasn't set
	QSettings *settingsSaver;
	bool complete = fileName.isEmpty();
	if(complete)
		settingsSaver = new QSettings(this);
	else
		settingsSaver = new QSettings(fileName, QSettings::IniFormat, this);
	if(settingsSaver->status() != QSettings::NoError)
		return -settingsSaver->status();

	if(complete) {
		// Main window layout and other general options
		settingsSaver->beginGroup("options");
		settingsSaver->beginGroup("window");
		// Docking windows and toolbars
		settingsSaver->beginGroup("docks");
		QList<DsoSettingsOptionsWindowPanel *> docks;
		docks.append(&(this->options.window.dock.horizontal));
		docks.append(&(this->options.window.dock.spectrum));
		docks.append(&(this->options.window.dock.trigger));
		docks.append(&(this->options.window.dock.voltage));
		QStringList dockNames;
		dockNames << "horizontal" << "spectrum" << "trigger" << "voltage";
		for(int dockId = 0; dockId < docks.size(); ++dockId) {
			settingsSaver->beginGroup(dockNames[dockId]);
			settingsSaver->setValue("floating", docks[dockId]->floating);
			settingsSaver->setValue("position", docks[dockId]->position);
			settingsSaver->setValue("visible", docks[dockId]->visible);
			settingsSaver->endGroup();
		}
		settingsSaver->endGroup();
		settingsSaver->beginGroup("toolbars");
		QList<DsoSettingsOptionsWindowPanel *> toolbars;
		toolbars.append(&(this->options.window.toolbar.file));
		toolbars.append(&(this->options.window.toolbar.oscilloscope));
		toolbars.append(&(this->options.window.toolbar.view));
		QStringList toolbarNames;
		toolbarNames << "file" << "oscilloscope" << "view";
		for(int toolbarId = 0; toolbarId < toolbars.size(); ++toolbarId) {
			settingsSaver->beginGroup(toolbarNames[toolbarId]);
			settingsSaver->setValue("floating", toolbars[toolbarId]->floating);
			settingsSaver->setValue("position", toolbars[toolbarId]->position);
			settingsSaver->setValue("visible", toolbars[toolbarId]->visible);
			settingsSaver->endGroup();
		}
		settingsSaver->endGroup();
		// Main window
		settingsSaver->setValue("pos", this->options.window.position);
		settingsSaver->setValue("size", this->options.window.size);
		settingsSaver->endGroup();
		settingsSaver->setValue("alwaysSave", this->options.alwaysSave);
		settingsSaver->setValue("imageSize", this->options.imageSize);
		settingsSaver->endGroup();
	}
	// Oszilloskope settings
	settingsSaver->beginGroup("scope");
	// Horizontal axis
	settingsSaver->beginGroup("horizontal");
	settingsSaver->setValue("format", this->scope.horizontal.format);
	settingsSaver->setValue("frequencybase", this->scope.horizontal.frequencybase);
	for(int marker = 0; marker < 2; ++marker)
		settingsSaver->setValue(QString("marker%1").arg(marker), this->scope.horizontal.marker[marker]);
	settingsSaver->setValue("timebase", this->scope.horizontal.timebase);
	settingsSaver->setValue("recordLength", this->scope.horizontal.recordLength);
	settingsSaver->setValue("samplerate", this->scope.horizontal.samplerate);
	settingsSaver->setValue("samplerateSet", this->scope.horizontal.samplerateSet);
	settingsSaver->endGroup();
	// Trigger
	settingsSaver->beginGroup("trigger");
	settingsSaver->setValue("filter", this->scope.trigger.filter);
	settingsSaver->setValue("mode", this->scope.trigger.mode);
	settingsSaver->setValue("position", this->scope.trigger.position);
	settingsSaver->setValue("slope", this->scope.trigger.slope);
	settingsSaver->setValue("source", this->scope.trigger.source);
	settingsSaver->setValue("special", this->scope.trigger.special);
	settingsSaver->endGroup();
	// Spectrum
	for(int channel = 0; channel < this->scope.spectrum.count(); ++channel) {
		settingsSaver->beginGroup(QString("spectrum%1").arg(channel));
		settingsSaver->setValue("magnitude", this->scope.spectrum[channel].magnitude);
		settingsSaver->setValue("offset", this->scope.spectrum[channel].offset);
		settingsSaver->setValue("used", this->scope.spectrum[channel].used);
		settingsSaver->endGroup();
	}
	// Vertical axis
	for(int channel = 0; channel < this->scope.voltage.count(); ++channel) {
		settingsSaver->beginGroup(QString("vertical%1").arg(channel));
		settingsSaver->setValue("gain", this->scope.voltage[channel].gain);
		settingsSaver->setValue("misc", this->scope.voltage[channel].misc);
		settingsSaver->setValue("offset", this->scope.voltage[channel].offset);
		settingsSaver->setValue("trigger", this->scope.voltage[channel].trigger);
		settingsSaver->setValue("used", this->scope.voltage[channel].used);
		settingsSaver->endGroup();
	}
	settingsSaver->setValue("spectrumLimit", this->scope.spectrumLimit);
	settingsSaver->setValue("spectrumReference", this->scope.spectrumReference);
	settingsSaver->setValue("spectrumWindow", this->scope.spectrumWindow);
	settingsSaver->endGroup();
	
	// View
	settingsSaver->beginGroup("view");
	// Colors
	if(complete) {
		settingsSaver->beginGroup("color");
		DsoSettingsColorValues *colors;
		for(int mode = 0; mode < 2; ++mode) {
			if(mode == 0) {
				colors = &this->view.color.screen;
				settingsSaver->beginGroup("screen");
			}
			else {
				colors = &this->view.color.print;
				settingsSaver->beginGroup("print");
			}
			
			settingsSaver->setValue("axes", colors->axes);
			settingsSaver->setValue("background", colors->background);
			settingsSaver->setValue("border", colors->border);
			settingsSaver->setValue("grid", colors->grid);
			settingsSaver->setValue("markers", colors->markers);
			for(int channel = 0; channel < this->scope.spectrum.count(); ++channel)
				settingsSaver->setValue(QString("spectrum%1").arg(channel), colors->spectrum[channel]);
			settingsSaver->setValue("text", colors->text);
			for(int channel = 0; channel < this->scope.voltage.count(); ++channel)
				settingsSaver->setValue(QString("voltage%1").arg(channel), colors->voltage[channel]);
			settingsSaver->endGroup();
		}
		settingsSaver->endGroup();
	}
	// Other view settings
	settingsSaver->setValue("digitalPhosphor", this->view.digitalPhosphor);
	if(complete) {
		settingsSaver->setValue("interpolation", this->view.interpolation);
		settingsSaver->setValue("screenColorImages", this->view.screenColorImages);
	}
	settingsSaver->setValue("zoom", this->view.zoom);
	settingsSaver->endGroup();
	
	delete settingsSaver;
	
	return 0;
}
