////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  settings.cpp
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


#include <QColor>


#include "settings.h"

#include "constants.h"
#include "dsowidget.h"


////////////////////////////////////////////////////////////////////////////////
// class DsoSettings
/// \brief Sets the values to their defaults.
DsoSettings::DsoSettings() {
	// Options
	this->options.alwaysSave = false;
	
	// Oscilloscope settings
	// Horizontal axis
	this->scope.horizontal.format = Dso::GRAPHFORMAT_TY;
	this->scope.horizontal.frequencybase = 1e3;
	this->scope.horizontal.marker[0] = -1.0;
	this->scope.horizontal.marker[1] = 1.0;
	this->scope.horizontal.timebase = 1e-3;
	this->scope.horizontal.samples = 10240;
	this->scope.horizontal.samplerate = 1e6;
	// Trigger
	this->scope.trigger.filter = true;
	this->scope.trigger.mode = Dso::TRIGGERMODE_NORMAL;
	this->scope.trigger.position = 0.0;
	this->scope.trigger.slope = Dso::SLOPE_POSITIVE;
	this->scope.trigger.source = 0;
	this->scope.trigger.special = false;
	// General
	this->scope.physicalChannels = 0;
	this->scope.spectrumLimit = 0.0;
	this->scope.spectrumReference = 20.0;
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
	this->view.interpolation = INTERPOLATION_LINEAR;
	this->view.screenColorImages = false;
	this->view.zoom = false;
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
	for(int channel = 0; channel < (int) channels; channel++) {
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
			newVoltage.used = (channel == 0);
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
			this->view.color.print.voltage.insert(channel, this->view.color.screen.voltage[channel]);
		if(this->view.color.print.spectrum.count() <= channel + 1)
			this->view.color.print.spectrum.insert(channel, this->view.color.print.voltage[channel].darker());
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

/// \brief Cleans up.
DsoSettings::~DsoSettings() {
}
