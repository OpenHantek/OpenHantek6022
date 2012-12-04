////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dsowidget.cpp
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


#include <cmath>

#include <QGridLayout>
#include <QTimer>


#include "dsowidget.h"

#include "dataanalyzer.h"
#include "dso.h"
#include "exporter.h"
#include "glscope.h"
#include "helper.h"
#include "levelslider.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// class DsoWidget
/// \brief Initializes the components of the oszilloscope-screen.
/// \param settings The settings object containing the oscilloscope settings.
/// \param dataAnalyzer The data analyzer that should be used as data source.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
DsoWidget::DsoWidget(DsoSettings *settings, DataAnalyzer *dataAnalyzer, QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags) {
	this->settings = settings;
	this->dataAnalyzer = dataAnalyzer;
	
	// Palette for this widget
	QPalette palette;
	palette.setColor(QPalette::Background, this->settings->view.color.screen.background);
	palette.setColor(QPalette::WindowText, this->settings->view.color.screen.text);
	
	// The OpenGL accelerated scope widgets
	this->generator = new GlGenerator(this->settings, this);
	this->generator->setDataAnalyzer(this->dataAnalyzer);
	this->mainScope = new GlScope(this->settings);
	this->mainScope->setGenerator(this->generator);
	this->zoomScope = new GlScope(this->settings);
	this->zoomScope->setGenerator(this->generator);
	this->zoomScope->setZoomMode(true);
	
#ifdef OS_DARWIN
	// Workaround for https://bugreports.qt-project.org/browse/QTBUG-8580
	this->mainScope->hide();
	this->mainScope->show();
#endif
	
	// The offset sliders for all possible channels
	this->offsetSlider = new LevelSlider(Qt::RightArrow);
	for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
		this->offsetSlider->addSlider(this->settings->scope.voltage[channel].name, channel);
		this->offsetSlider->setColor(channel, this->settings->view.color.screen.voltage[channel]);
		this->offsetSlider->setLimits(channel, -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2);
		this->offsetSlider->setStep(channel, 0.2);
		this->offsetSlider->setValue(channel, this->settings->scope.voltage[channel].offset);
		this->offsetSlider->setVisible(channel, this->settings->scope.voltage[channel].used);
	}
	for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
		this->offsetSlider->addSlider(this->settings->scope.spectrum[channel].name, this->settings->scope.voltage.count() + channel);
		this->offsetSlider->setColor(this->settings->scope.voltage.count() + channel, this->settings->view.color.screen.spectrum[channel]);
		this->offsetSlider->setLimits(this->settings->scope.voltage.count() + channel, -DIVS_VOLTAGE / 2, DIVS_VOLTAGE / 2);
		this->offsetSlider->setStep(this->settings->scope.voltage.count() + channel, 0.2);
		this->offsetSlider->setValue(this->settings->scope.voltage.count() + channel, this->settings->scope.spectrum[channel].offset);
		this->offsetSlider->setVisible(this->settings->scope.voltage.count() + channel, this->settings->scope.spectrum[channel].used);
	}
	
	// The triggerPosition slider
	this->triggerPositionSlider = new LevelSlider(Qt::DownArrow);
	this->triggerPositionSlider->addSlider();
	this->triggerPositionSlider->setLimits(0, 0.0, 1.0);
	this->triggerPositionSlider->setStep(0, 0.2 / DIVS_TIME);
	this->triggerPositionSlider->setValue(0, this->settings->scope.trigger.position);
	this->triggerPositionSlider->setVisible(0, true);
	
	// The sliders for the trigger levels
	this->triggerLevelSlider = new LevelSlider(Qt::LeftArrow);
	for(int channel = 0; channel < (int) this->settings->scope.physicalChannels; ++channel) {
		this->triggerLevelSlider->addSlider(channel);
		this->triggerLevelSlider->setColor(channel, (!this->settings->scope.trigger.special && channel == (int) this->settings->scope.trigger.source) ? this->settings->view.color.screen.voltage[channel] : this->settings->view.color.screen.voltage[channel].darker());
		this->adaptTriggerLevelSlider(channel);
		this->triggerLevelSlider->setValue(channel, this->settings->scope.voltage[channel].trigger);
		this->triggerLevelSlider->setVisible(channel, this->settings->scope.voltage[channel].used);
	}
	
	// The marker slider
	this->markerSlider = new LevelSlider(Qt::UpArrow);
	for(int marker = 0; marker < MARKER_COUNT; ++marker) {
		this->markerSlider->addSlider(QString::number(marker + 1), marker);
		this->markerSlider->setLimits(marker, -DIVS_TIME / 2, DIVS_TIME / 2);
		this->markerSlider->setStep(marker, 0.2);
		this->markerSlider->setValue(marker, this->settings->scope.horizontal.marker[marker]);
		this->markerSlider->setVisible(marker, true);
	}
	
	// The table for the settings
	this->settingsTriggerLabel = new QLabel();
	this->settingsTriggerLabel->setMinimumWidth(160);
	this->settingsRecordLengthLabel = new QLabel();
	this->settingsRecordLengthLabel->setAlignment(Qt::AlignRight);
	this->settingsRecordLengthLabel->setPalette(palette);
	this->settingsSamplerateLabel = new QLabel();
	this->settingsSamplerateLabel->setAlignment(Qt::AlignRight);
	this->settingsSamplerateLabel->setPalette(palette);
	this->settingsTimebaseLabel = new QLabel();
	this->settingsTimebaseLabel->setAlignment(Qt::AlignRight);
	this->settingsTimebaseLabel->setPalette(palette);
	this->settingsFrequencybaseLabel = new QLabel();
	this->settingsFrequencybaseLabel->setAlignment(Qt::AlignRight);
	this->settingsFrequencybaseLabel->setPalette(palette);
	this->settingsLayout = new QHBoxLayout();
	this->settingsLayout->addWidget(this->settingsTriggerLabel);
	this->settingsLayout->addWidget(this->settingsRecordLengthLabel, 1);
	this->settingsLayout->addWidget(this->settingsSamplerateLabel, 1);
	this->settingsLayout->addWidget(this->settingsTimebaseLabel, 1);
	this->settingsLayout->addWidget(this->settingsFrequencybaseLabel, 1);
	
	// The table for the marker details
	this->markerInfoLabel = new QLabel();
	this->markerInfoLabel->setMinimumWidth(160);
	this->markerInfoLabel->setPalette(palette);
	this->markerTimeLabel = new QLabel();
	this->markerTimeLabel->setAlignment(Qt::AlignRight);
	this->markerTimeLabel->setPalette(palette);
	this->markerFrequencyLabel = new QLabel();
	this->markerFrequencyLabel->setAlignment(Qt::AlignRight);
	this->markerFrequencyLabel->setPalette(palette);
	this->markerTimebaseLabel = new QLabel();
	this->markerTimebaseLabel->setAlignment(Qt::AlignRight);
	this->markerTimebaseLabel->setPalette(palette);
	this->markerFrequencybaseLabel = new QLabel();
	this->markerFrequencybaseLabel->setAlignment(Qt::AlignRight);
	this->markerFrequencybaseLabel->setPalette(palette);
	this->markerLayout = new QHBoxLayout();
	this->markerLayout->addWidget(this->markerInfoLabel);
	this->markerLayout->addWidget(this->markerTimeLabel, 1);
	this->markerLayout->addWidget(this->markerFrequencyLabel, 1);
	this->markerLayout->addWidget(this->markerTimebaseLabel, 1);
	this->markerLayout->addWidget(this->markerFrequencybaseLabel, 1);
	
	// The table for the measurements
	QPalette tablePalette = palette;
	this->measurementLayout = new QGridLayout();
	this->measurementLayout->setColumnMinimumWidth(0, 64);
	this->measurementLayout->setColumnMinimumWidth(1, 32);
	this->measurementLayout->setColumnStretch(2, 2);
	this->measurementLayout->setColumnStretch(3, 2);
	this->measurementLayout->setColumnStretch(4, 3);
	this->measurementLayout->setColumnStretch(5, 3);
	for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
		tablePalette.setColor(QPalette::WindowText, this->settings->view.color.screen.voltage[channel]);
		this->measurementNameLabel.append(new QLabel(this->settings->scope.voltage[channel].name));
		this->measurementNameLabel[channel]->setPalette(tablePalette);
		this->measurementMiscLabel.append(new QLabel());
		this->measurementMiscLabel[channel]->setPalette(tablePalette);
		this->measurementGainLabel.append(new QLabel());
		this->measurementGainLabel[channel]->setAlignment(Qt::AlignRight);
		this->measurementGainLabel[channel]->setPalette(tablePalette);
		tablePalette.setColor(QPalette::WindowText, this->settings->view.color.screen.spectrum[channel]);
		this->measurementMagnitudeLabel.append(new QLabel());
		this->measurementMagnitudeLabel[channel]->setAlignment(Qt::AlignRight);
		this->measurementMagnitudeLabel[channel]->setPalette(tablePalette);
		this->measurementAmplitudeLabel.append(new QLabel());
		this->measurementAmplitudeLabel[channel]->setAlignment(Qt::AlignRight);
		this->measurementAmplitudeLabel[channel]->setPalette(palette);
		this->measurementFrequencyLabel.append(new QLabel());
		this->measurementFrequencyLabel[channel]->setAlignment(Qt::AlignRight);
		this->measurementFrequencyLabel[channel]->setPalette(palette);
		this->setMeasurementVisible(channel, this->settings->scope.voltage[channel].used);
		this->measurementLayout->addWidget(this->measurementNameLabel[channel], channel, 0);
		this->measurementLayout->addWidget(this->measurementMiscLabel[channel], channel, 1);
		this->measurementLayout->addWidget(this->measurementGainLabel[channel], channel, 2);
		this->measurementLayout->addWidget(this->measurementMagnitudeLabel[channel], channel, 3);
		this->measurementLayout->addWidget(this->measurementAmplitudeLabel[channel], channel, 4);
		this->measurementLayout->addWidget(this->measurementFrequencyLabel[channel], channel, 5);
		if((unsigned int) channel < this->settings->scope.physicalChannels)
			this->updateVoltageCoupling(channel);
		else
			this->updateMathMode();
		this->updateVoltageDetails(channel);
		this->updateSpectrumDetails(channel);
	}
	
	// The layout for the widgets
	this->mainLayout = new QGridLayout();
	this->mainLayout->setColumnStretch(2, 1); // Scopes increase their size
	this->mainLayout->setRowStretch(3, 1);
	// Bars around the scope, needed because the slider-drawing-area is outside
	// the scope at min/max
	this->mainLayout->setColumnMinimumWidth(1, this->triggerPositionSlider->preMargin());
	this->mainLayout->setColumnMinimumWidth(3, this->triggerPositionSlider->postMargin());
	this->mainLayout->setRowMinimumHeight(2, this->offsetSlider->preMargin());
	this->mainLayout->setRowMinimumHeight(4, this->offsetSlider->postMargin());
	this->mainLayout->setRowMinimumHeight(6, 4);
	this->mainLayout->setRowMinimumHeight(8, 4);
	this->mainLayout->setRowMinimumHeight(10, 8);
	this->mainLayout->setSpacing(0);
	this->mainLayout->addLayout(this->settingsLayout, 0, 0, 1, 5);
	this->mainLayout->addWidget(this->mainScope, 3, 2);
	this->mainLayout->addWidget(this->offsetSlider, 2, 0, 3, 2, Qt::AlignRight);
	this->mainLayout->addWidget(this->triggerPositionSlider, 1, 1, 2, 3, Qt::AlignBottom);
	this->mainLayout->addWidget(this->triggerLevelSlider, 2, 3, 3, 2, Qt::AlignLeft);
	this->mainLayout->addWidget(this->markerSlider, 4, 1, 2, 3, Qt::AlignTop);
	this->mainLayout->addLayout(this->markerLayout, 7, 0, 1, 5);
	this->mainLayout->addWidget(this->zoomScope, 9, 2);
	this->mainLayout->addLayout(this->measurementLayout, 11, 0, 1, 5);
	
	// Apply settings and update measured values
	this->updateTriggerDetails();
	this->updateRecordLength(this->settings->scope.horizontal.recordLength);
	this->updateFrequencybase(this->settings->scope.horizontal.frequencybase);
	this->updateSamplerate(this->settings->scope.horizontal.samplerate);
	this->updateTimebase(this->settings->scope.horizontal.timebase);
	this->updateZoom(this->settings->view.zoom);
	
	// The widget itself
	this->setPalette(palette);
	this->setBackgroundRole(QPalette::Background);
	this->setAutoFillBackground(true);
	this->setLayout(this->mainLayout);
	
	// Connect change-signals of sliders
	this->connect(this->offsetSlider, SIGNAL(valueChanged(int, double)), this, SLOT(updateOffset(int, double)));
	this->connect(this->triggerPositionSlider, SIGNAL(valueChanged(int, double)), this, SLOT(updateTriggerPosition(int, double)));
	this->connect(this->triggerLevelSlider, SIGNAL(valueChanged(int, double)), this, SLOT(updateTriggerLevel(int, double)));
	this->connect(this->markerSlider, SIGNAL(valueChanged(int, double)), this, SLOT(updateMarker(int, double)));
	this->connect(this->markerSlider, SIGNAL(valueChanged(int, double)), this->mainScope, SLOT(updateGL()));
	this->connect(this->markerSlider, SIGNAL(valueChanged(int, double)), this->zoomScope, SLOT(updateGL()));
	
	// Connect other signals
	this->connect(this->dataAnalyzer, SIGNAL(analyzed(unsigned long)), this, SLOT(dataAnalyzed()));
	this->connect(this->dataAnalyzer, SIGNAL(analyzed(unsigned long)), this, SLOT(updateRecordLength(unsigned long)));
}

/// \brief Stops the oscilloscope thread and the timer.
DsoWidget::~DsoWidget() {
}

/// \brief Set the trigger level sliders minimum and maximum to the new values.
void DsoWidget::adaptTriggerLevelSlider(unsigned int channel) {
	this->triggerLevelSlider->setLimits(channel, (-DIVS_VOLTAGE / 2 - this->settings->scope.voltage[channel].offset) * this->settings->scope.voltage[channel].gain, (DIVS_VOLTAGE / 2 - this->settings->scope.voltage[channel].offset) * this->settings->scope.voltage[channel].gain);
	this->triggerLevelSlider->setStep(channel, this->settings->scope.voltage[channel].gain * 0.2);
}

/// \brief Show/Hide a line of the measurement table.
void DsoWidget::setMeasurementVisible(unsigned int channel, bool visible) {
	this->measurementNameLabel[channel]->setVisible(visible);
	this->measurementMiscLabel[channel]->setVisible(visible);
	this->measurementGainLabel[channel]->setVisible(visible);
	this->measurementMagnitudeLabel[channel]->setVisible(visible);
	this->measurementAmplitudeLabel[channel]->setVisible(visible);
	this->measurementFrequencyLabel[channel]->setVisible(visible);
	if(!visible) {
		this->measurementGainLabel[channel]->setText(QString());
		this->measurementMagnitudeLabel[channel]->setText(QString());
		this->measurementAmplitudeLabel[channel]->setText(QString());
		this->measurementFrequencyLabel[channel]->setText(QString());
	}
}

/// \brief Update the label about the marker measurements
void DsoWidget::updateMarkerDetails() {
	double divs = fabs(this->settings->scope.horizontal.marker[1] - this->settings->scope.horizontal.marker[0]);
	double time = divs * this->settings->scope.horizontal.timebase;
	
	if(this->settings->view.zoom) {
		this->markerInfoLabel->setText(tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3));
		this->markerTimebaseLabel->setText(Helper::valueToString(time / DIVS_TIME, Helper::UNIT_SECONDS, 3) + tr("/div"));
		this->markerFrequencybaseLabel->setText(Helper::valueToString(divs  * this->settings->scope.horizontal.frequencybase / DIVS_TIME, Helper::UNIT_HERTZ, 4) + tr("/div"));
	}
	this->markerTimeLabel->setText(Helper::valueToString(time, Helper::UNIT_SECONDS, 4));
	this->markerFrequencyLabel->setText(Helper::valueToString(1.0 / time, Helper::UNIT_HERTZ, 4));
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateSpectrumDetails(unsigned int channel) {
	this->setMeasurementVisible(channel, this->settings->scope.voltage[channel].used || this->settings->scope.spectrum[channel].used);
	
	if(this->settings->scope.spectrum[channel].used)
		this->measurementMagnitudeLabel[channel]->setText(Helper::valueToString(this->settings->scope.spectrum[channel].magnitude, Helper::UNIT_DECIBEL, 3) + tr("/div"));
	else
		this->measurementMagnitudeLabel[channel]->setText(QString());
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateTriggerDetails() {
	// Update the trigger details
	QPalette tablePalette = this->palette();
	tablePalette.setColor(QPalette::WindowText, this->settings->view.color.screen.voltage[this->settings->scope.trigger.source]);
	this->settingsTriggerLabel->setPalette(tablePalette);
	QString levelString = Helper::valueToString(this->settings->scope.voltage[this->settings->scope.trigger.source].trigger, Helper::UNIT_VOLTS, 3);
	QString pretriggerString = tr("%L1%").arg((int) (this->settings->scope.trigger.position * 100 + 0.5));
	this->settingsTriggerLabel->setText(tr("%1  %2  %3  %4").arg(this->settings->scope.voltage[this->settings->scope.trigger.source].name, Dso::slopeString(this->settings->scope.trigger.slope), levelString, pretriggerString));
	
	/// \todo This won't work for special trigger sources
}

/// \brief Update the label about the trigger settings
void DsoWidget::updateVoltageDetails(unsigned int channel) {
	if(channel >= (unsigned int) this->settings->scope.voltage.count())
		return;
	
	this->setMeasurementVisible(channel, this->settings->scope.voltage[channel].used || this->settings->scope.spectrum[channel].used);
	
	if(this->settings->scope.voltage[channel].used)
		this->measurementGainLabel[channel]->setText(Helper::valueToString(this->settings->scope.voltage[channel].gain, Helper::UNIT_VOLTS, 3) + tr("/div"));
	else
		this->measurementGainLabel[channel]->setText(QString());
}

/// \brief Handles frequencybaseChanged signal from the horizontal dock.
/// \param frequencybase The frequencybase used for displaying the trace.
void DsoWidget::updateFrequencybase(double frequencybase) {
	this->settingsFrequencybaseLabel->setText(Helper::valueToString(frequencybase, Helper::UNIT_HERTZ, 4) + tr("/div"));
}

/// \brief Updates the samplerate field after changing the samplerate.
/// \param samplerate The samplerate set in the oscilloscope.
void DsoWidget::updateSamplerate(double samplerate) {
	this->settingsSamplerateLabel->setText(Helper::valueToString(samplerate, Helper::UNIT_SAMPLES, 4) + tr("/s"));
}

/// \brief Handles timebaseChanged signal from the horizontal dock.
/// \param timebase The timebase used for displaying the trace.
void DsoWidget::updateTimebase(double timebase) {
	this->settingsTimebaseLabel->setText(Helper::valueToString(timebase, Helper::UNIT_SECONDS, 4) + tr("/div"));
	
	this->updateMarkerDetails();
}

/// \brief Handles magnitudeChanged signal from the spectrum dock.
/// \param channel The channel whose magnitude was changed.
void DsoWidget::updateSpectrumMagnitude(unsigned int channel) {
	this->updateSpectrumDetails(channel);
}

/// \brief Handles usedChanged signal from the spectrum dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateSpectrumUsed(unsigned int channel, bool used) {
	if(channel >= (unsigned int) this->settings->scope.voltage.count())
		return;
	
	this->offsetSlider->setVisible(this->settings->scope.voltage.count() + channel, used);
	
	this->updateSpectrumDetails(channel);
}

/// \brief Handles modeChanged signal from the trigger dock.
void DsoWidget::updateTriggerMode() {
	this->updateTriggerDetails();
}

/// \brief Handles slopeChanged signal from the trigger dock.
void DsoWidget::updateTriggerSlope() {
	this->updateTriggerDetails();
}

/// \brief Handles sourceChanged signal from the trigger dock.
void DsoWidget::updateTriggerSource() {
	// Change the colors of the trigger sliders
	if(this->settings->scope.trigger.special || this->settings->scope.trigger.source >= this->settings->scope.physicalChannels)
		this->triggerPositionSlider->setColor(0, this->settings->view.color.screen.border);
	else
		this->triggerPositionSlider->setColor(0, this->settings->view.color.screen.voltage[this->settings->scope.trigger.source]);
	
	for(int channel = 0; channel < (int) this->settings->scope.physicalChannels; ++channel)
		this->triggerLevelSlider->setColor(channel, (!this->settings->scope.trigger.special && channel == (int) this->settings->scope.trigger.source) ? this->settings->view.color.screen.voltage[channel] : this->settings->view.color.screen.voltage[channel].darker());
	
	this->updateTriggerDetails();
}

/// \brief Handles couplingChanged signal from the voltage dock.
/// \param channel The channel whose coupling was changed.
void DsoWidget::updateVoltageCoupling(unsigned int channel) {
	if(channel >= (unsigned int) this->settings->scope.voltage.count())
		return;
	
	this->measurementMiscLabel[channel]->setText(Dso::couplingString((Dso::Coupling) this->settings->scope.voltage[channel].misc));
}

/// \brief Handles modeChanged signal from the voltage dock.
void DsoWidget::updateMathMode() {
	this->measurementMiscLabel[this->settings->scope.physicalChannels]->setText(Dso::mathModeString((Dso::MathMode) this->settings->scope.voltage[this->settings->scope.physicalChannels].misc));
}

/// \brief Handles gainChanged signal from the voltage dock.
/// \param channel The channel whose gain was changed.
void DsoWidget::updateVoltageGain(unsigned int channel) {
	if(channel >= (unsigned int) this->settings->scope.voltage.count())
		return;
	
	if(channel < this->settings->scope.physicalChannels)
		this->adaptTriggerLevelSlider(channel);
	
	this->updateVoltageDetails(channel);
}

/// \brief Handles usedChanged signal from the voltage dock.
/// \param channel The channel whose used-state was changed.
/// \param used The new used-state for the channel.
void DsoWidget::updateVoltageUsed(unsigned int channel, bool used) {
	if(channel >= (unsigned int) this->settings->scope.voltage.count())
		return;
	
	this->offsetSlider->setVisible(channel, used);
	this->triggerLevelSlider->setVisible(channel, used);
	this->setMeasurementVisible(channel, this->settings->scope.voltage[channel].used);
	
	this->updateVoltageDetails(channel);
}

/// \brief Change the record length.
void DsoWidget::updateRecordLength(unsigned long size) {
	this->settingsRecordLengthLabel->setText(Helper::valueToString(size, Helper::UNIT_SAMPLES, 4));
}

/// \brief Export the oscilloscope screen to a file.
/// \return true if the document was exported successfully.
bool DsoWidget::exportAs() {
	QStringList filters;
	filters
			<< tr("Portable Document Format (*.pdf)")
			<< tr("PostScript (*.ps)")
			<< tr("Image (*.png *.xpm *.jpg)")
			<< tr("Comma-Separated Values (*.csv)");
	
	QFileDialog fileDialog(static_cast<QWidget *>(this->parent()), tr("Export file..."), QString(), filters.join(";;"));
	fileDialog.setFileMode(QFileDialog::AnyFile);
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	if(fileDialog.exec() != QDialog::Accepted)
		return false;
	
	Exporter exporter(this->settings, this->dataAnalyzer, static_cast<QWidget *>(this->parent()));
	exporter.setFilename(fileDialog.selectedFiles().first());
	exporter.setFormat((ExportFormat) (EXPORT_FORMAT_PDF + filters.indexOf(fileDialog.selectedFilter())));
	
	return exporter.doExport();
}

/// \brief Print the oscilloscope screen.
/// \return true if the document was sent to the printer successfully.
bool DsoWidget::print() {
	Exporter exporter(this->settings, this->dataAnalyzer, static_cast<QWidget *>(this->parent()));
	exporter.setFormat(EXPORT_FORMAT_PRINTER);
	
	return exporter.doExport();
}

/// \brief Stop the oscilloscope.
void DsoWidget::updateZoom(bool enabled) {
	this->mainLayout->setRowStretch(9, enabled ? 1 : 0);
	this->zoomScope->setVisible(enabled);
	
	// Show time-/frequencybase and zoom factor if the magnified scope is shown
	this->markerLayout->setStretch(3, enabled ? 1 : 0);
	this->markerTimebaseLabel->setVisible(enabled);
	this->markerLayout->setStretch(4, enabled ? 1 : 0);
	this->markerFrequencybaseLabel->setVisible(enabled);
	if(enabled)
		this->updateMarkerDetails();
	else
		this->markerInfoLabel->setText(tr("Marker 1/2"));
	
	this->repaint();
}

/// \brief Prints analyzed data.
void DsoWidget::dataAnalyzed() {
	for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
		if(this->settings->scope.voltage[channel].used && this->dataAnalyzer->data(channel)) {			
			// Amplitude string representation (4 significant digits)
			this->measurementAmplitudeLabel[channel]->setText(Helper::valueToString(this->dataAnalyzer->data(channel)->amplitude, Helper::UNIT_VOLTS, 4));
			// Frequency string representation (5 significant digits)
			this->measurementFrequencyLabel[channel]->setText(Helper::valueToString(this->dataAnalyzer->data(channel)->frequency, Helper::UNIT_HERTZ, 5));
		}
	}
}

/// \brief Handles valueChanged signal from the offset sliders.
/// \param channel The channel whose offset was changed.
/// \param value The new offset for the channel.
void DsoWidget::updateOffset(int channel, double value) {
	if(channel < this->settings->scope.voltage.count()) {
		this->settings->scope.voltage[channel].offset = value;
		
		if(channel < (int) this->settings->scope.physicalChannels)
			this->adaptTriggerLevelSlider(channel);
	}
	else if(channel < this->settings->scope.voltage.count() * 2)
		this->settings->scope.spectrum[channel - this->settings->scope.voltage.count()].offset = value;
	
	emit offsetChanged(channel, value);
}

/// \brief Handles valueChanged signal from the triggerPosition slider.
/// \param index The index of the slider.
/// \param value The new triggerPosition in seconds relative to the first sample.
void DsoWidget::updateTriggerPosition(int index, double value) {
	if(index != 0)
		return;
	
	this->settings->scope.trigger.position = value;
	
	this->updateTriggerDetails();
	
	emit triggerPositionChanged(value * this->settings->scope.horizontal.timebase * DIVS_TIME);
}

/// \brief Handles valueChanged signal from the trigger level slider.
/// \param channel The index of the slider.
/// \param value The new trigger level.
void DsoWidget::updateTriggerLevel(int channel, double value) {
	this->settings->scope.voltage[channel].trigger = value;
	
	this->updateTriggerDetails();
	
	emit triggerLevelChanged(channel, value);
}

/// \brief Handles valueChanged signal from the marker slider.
/// \param marker The index of the slider.
/// \param value The new marker position.
void DsoWidget::updateMarker(int marker, double value) {
	this->settings->scope.horizontal.marker[marker] = value;
	
	this->updateMarkerDetails();
	
	emit markerChanged(marker, value);
}
