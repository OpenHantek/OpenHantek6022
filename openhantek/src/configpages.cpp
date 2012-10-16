////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  configpages.cpp
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


#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>


#include "configpages.h"

#include "colorbox.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// class DsoConfigAnalysisPage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigAnalysisPage::DsoConfigAnalysisPage(DsoSettings *settings, QWidget *parent) : QWidget(parent) {
	this->settings = settings;
	
	// Initialize lists for comboboxes
	QStringList windowFunctionStrings;
	windowFunctionStrings
			<< tr("Rectangular")
			<< tr("Hamming")
			<< tr("Hann")
			<< tr("Cosine")
			<< tr("Lanczos")
			<< tr("Bartlett")
			<< tr("Triangular")
			<< tr("Gauss")
			<< tr("Bartlett-Hann")
			<< tr("Blackman")
			//<< tr("Kaiser")
			<< tr("Nuttall")
			<< tr("Blackman-Harris")
			<< tr("Blackman-Nuttall")
			<< tr("Flat top");
	
	// Initialize elements
	this->windowFunctionLabel = new QLabel(tr("Window function"));
	this->windowFunctionComboBox = new QComboBox();
	this->windowFunctionComboBox->addItems(windowFunctionStrings);
	this->windowFunctionComboBox->setCurrentIndex(this->settings->scope.spectrumWindow);
	
	this->referenceLevelLabel = new QLabel(tr("Reference level"));
	this->referenceLevelSpinBox = new QDoubleSpinBox();
	this->referenceLevelSpinBox->setDecimals(1);
	this->referenceLevelSpinBox->setMinimum(-40.0);
	this->referenceLevelSpinBox->setMaximum(100.0);
	this->referenceLevelSpinBox->setValue(this->settings->scope.spectrumReference);
	this->referenceLevelUnitLabel = new QLabel(tr("dBm"));
	this->referenceLevelLayout = new QHBoxLayout();
	this->referenceLevelLayout->addWidget(this->referenceLevelSpinBox);
	this->referenceLevelLayout->addWidget(this->referenceLevelUnitLabel);
	
	this->minimumMagnitudeLabel = new QLabel(tr("Minimum magnitude"));
	this->minimumMagnitudeSpinBox = new QDoubleSpinBox();
	this->minimumMagnitudeSpinBox->setDecimals(1);
	this->minimumMagnitudeSpinBox->setMinimum(-40.0);
	this->minimumMagnitudeSpinBox->setMaximum(100.0);
	this->minimumMagnitudeSpinBox->setValue(this->settings->scope.spectrumLimit);
	this->minimumMagnitudeUnitLabel = new QLabel(tr("dBm"));
	this->minimumMagnitudeLayout = new QHBoxLayout();
	this->minimumMagnitudeLayout->addWidget(this->minimumMagnitudeSpinBox);
	this->minimumMagnitudeLayout->addWidget(this->minimumMagnitudeUnitLabel);
	
	this->spectrumLayout = new QGridLayout();
	this->spectrumLayout->addWidget(this->windowFunctionLabel, 0, 0);
	this->spectrumLayout->addWidget(this->windowFunctionComboBox, 0, 1);
	this->spectrumLayout->addWidget(this->referenceLevelLabel, 1, 0);
	this->spectrumLayout->addLayout(this->referenceLevelLayout, 1, 1);
	this->spectrumLayout->addWidget(this->minimumMagnitudeLabel, 2, 0);
	this->spectrumLayout->addLayout(this->minimumMagnitudeLayout, 2, 1);
	
	this->spectrumGroup = new QGroupBox(tr("Spectrum"));
	this->spectrumGroup->setLayout(this->spectrumLayout);
	
	this->mainLayout = new QVBoxLayout();
	this->mainLayout->addWidget(this->spectrumGroup);
	this->mainLayout->addStretch(1);
	
	this->setLayout(this->mainLayout);
}

/// \brief Cleans up the widget.
DsoConfigAnalysisPage::~DsoConfigAnalysisPage() {
}

/// \brief Saves the new settings.
void DsoConfigAnalysisPage::saveSettings() {
	this->settings->scope.spectrumWindow = (Dso::WindowFunction) this->windowFunctionComboBox->currentIndex();
	this->settings->scope.spectrumReference = this->referenceLevelSpinBox->value();
	this->settings->scope.spectrumLimit = this->minimumMagnitudeSpinBox->value();
}


////////////////////////////////////////////////////////////////////////////////
// class DsoConfigColorsPage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigColorsPage::DsoConfigColorsPage(DsoSettings *settings, QWidget *parent) : QWidget(parent) {
	this->settings = settings;
	
	// Initialize elements
	// Screen category
	this->axesLabel = new QLabel(tr("Axes"));
	this->axesColorBox = new ColorBox(this->settings->view.color.screen.axes);
	this->backgroundLabel = new QLabel(tr("Background"));
	this->backgroundColorBox = new ColorBox(this->settings->view.color.screen.background);
	this->borderLabel = new QLabel(tr("Border"));
	this->borderColorBox = new ColorBox(this->settings->view.color.screen.border);
	this->gridLabel = new QLabel(tr("Grid"));
	this->gridColorBox = new ColorBox(this->settings->view.color.screen.grid);
	this->markersLabel = new QLabel(tr("Markers"));
	this->markersColorBox = new ColorBox(this->settings->view.color.screen.markers);
	this->textLabel = new QLabel(tr("Text"));
	this->textColorBox = new ColorBox(this->settings->view.color.screen.text);
	
	this->screenLayout = new QGridLayout();
	this->screenLayout->setColumnStretch(0, 1);
	this->screenLayout->setColumnMinimumWidth(1, 80);
	this->screenLayout->addWidget(this->backgroundLabel, 0, 0);
	this->screenLayout->addWidget(this->backgroundColorBox, 0, 1);
	this->screenLayout->addWidget(this->gridLabel, 1, 0);
	this->screenLayout->addWidget(this->gridColorBox, 1, 1);
	this->screenLayout->addWidget(this->axesLabel, 2, 0);
	this->screenLayout->addWidget(this->axesColorBox, 2, 1);
	this->screenLayout->addWidget(this->borderLabel, 3, 0);
	this->screenLayout->addWidget(this->borderColorBox, 3, 1);
	this->screenLayout->addWidget(this->markersLabel, 4, 0);
	this->screenLayout->addWidget(this->markersColorBox, 4, 1);
	this->screenLayout->addWidget(this->textLabel, 5, 0);
	this->screenLayout->addWidget(this->textColorBox, 5, 1);
	
	this->screenGroup = new QGroupBox(tr("Screen"));
	this->screenGroup->setLayout(this->screenLayout);
	
	// Graph category
	this->channelLabel = new QLabel(tr("Channel"));
	this->channelLabel->setAlignment(Qt::AlignHCenter);
	this->spectrumLabel = new QLabel(tr("Spectrum"));
	this->spectrumLabel->setAlignment(Qt::AlignHCenter);
	for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
		this->colorLabel.append(new QLabel(this->settings->scope.voltage[channel].name));
		this->channelColorBox.append(new ColorBox(this->settings->view.color.screen.voltage[channel]));
		this->spectrumColorBox.append(new ColorBox(this->settings->view.color.screen.spectrum[channel]));
	}
	
	this->graphLayout = new QGridLayout();
	this->graphLayout->setColumnStretch(0, 1);
	this->graphLayout->setColumnMinimumWidth(1, 80);
	this->graphLayout->setColumnMinimumWidth(2, 80);
	this->graphLayout->addWidget(this->channelLabel, 0, 1);
	this->graphLayout->addWidget(this->spectrumLabel, 0, 2);
	for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
		this->graphLayout->addWidget(this->colorLabel[channel], channel + 1, 0);
		this->graphLayout->addWidget(this->channelColorBox[channel], channel + 1, 1);
		this->graphLayout->addWidget(this->spectrumColorBox[channel], channel + 1, 2);
	}
	
	this->graphGroup = new QGroupBox(tr("Graph"));
	this->graphGroup->setLayout(this->graphLayout);
	
	// Main layout
	this->mainLayout = new QVBoxLayout();
	this->mainLayout->addWidget(this->screenGroup);
	this->mainLayout->addWidget(this->graphGroup);
	this->mainLayout->addStretch(1);
	
	this->setLayout(this->mainLayout);
}

/// \brief Cleans up the widget.
DsoConfigColorsPage::~DsoConfigColorsPage() {
}

/// \brief Saves the new settings.
void DsoConfigColorsPage::saveSettings() {
	// Screen category
	this->settings->view.color.screen.axes = this->axesColorBox->getColor();
	this->settings->view.color.screen.background = this->backgroundColorBox->getColor();
	this->settings->view.color.screen.border = this->borderColorBox->getColor();
	this->settings->view.color.screen.grid = this->gridColorBox->getColor();
	this->settings->view.color.screen.markers = this->markersColorBox->getColor();
	this->settings->view.color.screen.text = this->textColorBox->getColor();
	
	// Graph category
	for(int channel = 0; channel < this->settings->scope.voltage.count(); ++channel) {
		this->settings->view.color.screen.voltage[channel] = this->channelColorBox[channel]->getColor();
		this->settings->view.color.screen.spectrum[channel] = this->spectrumColorBox[channel]->getColor();
	}
}


////////////////////////////////////////////////////////////////////////////////
// class DsoConfigFilesPage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigFilesPage::DsoConfigFilesPage(DsoSettings *settings, QWidget *parent) : QWidget(parent) {
	this->settings = settings;
	
	// Initialize elements
	this->saveOnExitCheckBox = new QCheckBox(tr("Save default settings on exit"));
	this->saveOnExitCheckBox->setChecked(this->settings->options.alwaysSave);
	this->saveNowButton = new QPushButton(tr("Save default settings now"));
	
	this->configurationLayout = new QVBoxLayout();
	this->configurationLayout->addWidget(this->saveOnExitCheckBox, 0);
	this->configurationLayout->addWidget(this->saveNowButton, 1);
	
	this->configurationGroup = new QGroupBox(tr("Configuration"));
	this->configurationGroup->setLayout(this->configurationLayout);
	
	this->imageWidthLabel = new QLabel(tr("Image width"));
	this->imageWidthSpinBox = new QSpinBox();
	this->imageWidthSpinBox->setMinimum(100);
	this->imageWidthSpinBox->setMaximum(9999);
	this->imageWidthSpinBox->setValue(this->settings->options.imageSize.width());
	this->imageHeightLabel = new QLabel(tr("Image height"));
	this->imageHeightSpinBox = new QSpinBox();
	this->imageHeightSpinBox->setMinimum(100);
	this->imageHeightSpinBox->setMaximum(9999);
	this->imageHeightSpinBox->setValue(this->settings->options.imageSize.height());
	
	this->exportLayout = new QGridLayout();
	this->exportLayout->addWidget(this->imageWidthLabel, 0, 0);
	this->exportLayout->addWidget(this->imageWidthSpinBox, 0, 1);
	this->exportLayout->addWidget(this->imageHeightLabel, 1, 0);
	this->exportLayout->addWidget(this->imageHeightSpinBox, 1, 1);
	
	this->exportGroup = new QGroupBox(tr("Export"));
	this->exportGroup->setLayout(this->exportLayout);
	
	this->mainLayout = new QVBoxLayout();
	this->mainLayout->addWidget(this->configurationGroup);
	this->mainLayout->addWidget(this->exportGroup);
	this->mainLayout->addStretch(1);
	
	this->setLayout(this->mainLayout);
	
	connect(this->saveNowButton, SIGNAL(clicked()), this->settings, SLOT(save()));
}

/// \brief Cleans up the widget.
DsoConfigFilesPage::~DsoConfigFilesPage() {
}

/// \brief Saves the new settings.
void DsoConfigFilesPage::saveSettings() {
	this->settings->options.alwaysSave = this->saveOnExitCheckBox->isChecked();
	this->settings->options.imageSize.setWidth(this->imageWidthSpinBox->value());
	this->settings->options.imageSize.setHeight(this->imageHeightSpinBox->value());
}


////////////////////////////////////////////////////////////////////////////////
// class DsoConfigScopePage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigScopePage::DsoConfigScopePage(DsoSettings *settings, QWidget *parent) : QWidget(parent) {
	this->settings = settings;
	
	// Initialize lists for comboboxes
	QStringList interpolationStrings;
	interpolationStrings
			<< tr("Off")
			<< tr("Linear")
			<< tr("Sinc");
	
	// Initialize elements
	this->antialiasingCheckBox = new QCheckBox(tr("Antialiasing"));
	this->antialiasingCheckBox->setChecked(this->settings->view.antialiasing);
	this->interpolationLabel = new QLabel(tr("Interpolation"));
	this->interpolationComboBox = new QComboBox();
	this->interpolationComboBox->addItems(interpolationStrings);
	this->interpolationComboBox->setCurrentIndex(this->settings->view.interpolation);
	this->digitalPhosphorDepthLabel = new QLabel(tr("Digital phosphor depth"));
	this->digitalPhosphorDepthSpinBox = new QSpinBox();
	this->digitalPhosphorDepthSpinBox->setMinimum(2);
	this->digitalPhosphorDepthSpinBox->setMaximum(99);
	this->digitalPhosphorDepthSpinBox->setValue(this->settings->view.digitalPhosphorDepth);
	
	this->graphLayout = new QGridLayout();
	this->graphLayout->addWidget(this->antialiasingCheckBox, 0, 0, 1, 2);
	this->graphLayout->addWidget(this->interpolationLabel, 1, 0);
	this->graphLayout->addWidget(this->interpolationComboBox, 1, 1);
	this->graphLayout->addWidget(this->digitalPhosphorDepthLabel, 2, 0);
	this->graphLayout->addWidget(this->digitalPhosphorDepthSpinBox, 2, 1);
	
	this->graphGroup = new QGroupBox(tr("Graph"));
	this->graphGroup->setLayout(this->graphLayout);
	
	this->mainLayout = new QVBoxLayout();
	this->mainLayout->addWidget(this->graphGroup);
	this->mainLayout->addStretch(1);
	
	this->setLayout(this->mainLayout);
}

/// \brief Cleans up the widget.
DsoConfigScopePage::~DsoConfigScopePage() {
}

/// \brief Saves the new settings.
void DsoConfigScopePage::saveSettings() {
	this->settings->view.antialiasing = this->antialiasingCheckBox->isChecked();
	this->settings->view.interpolation = (Dso::InterpolationMode) this->interpolationComboBox->currentIndex();
	this->settings->view.digitalPhosphorDepth = this->digitalPhosphorDepthSpinBox->value();
}
