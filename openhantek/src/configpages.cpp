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
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
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
DsoConfigAnalysisPage::DsoConfigAnalysisPage(DsoSettings *settings,
                                             QWidget *parent)
    : QWidget(parent) {
  this->settings = settings;

  // Initialize lists for comboboxes
  QStringList windowFunctionStrings;
  windowFunctionStrings << tr("Rectangular") << tr("Hamming") << tr("Hann")
                        << tr("Cosine") << tr("Lanczos") << tr("Bartlett")
                        << tr("Triangular") << tr("Gauss")
                        << tr("Bartlett-Hann") << tr("Blackman")
                        //<< tr("Kaiser")
                        << tr("Nuttall") << tr("Blackman-Harris")
                        << tr("Blackman-Nuttall") << tr("Flat top");

  // Initialize elements
  this->windowFunctionLabel = new QLabel(tr("Window function"));
  this->windowFunctionComboBox = new QComboBox();
  this->windowFunctionComboBox->addItems(windowFunctionStrings);
  this->windowFunctionComboBox->setCurrentIndex(
      this->settings->scope.spectrumWindow);

  this->referenceLevelLabel = new QLabel(tr("Reference level"));
  this->referenceLevelSpinBox = new QDoubleSpinBox();
  this->referenceLevelSpinBox->setDecimals(1);
  this->referenceLevelSpinBox->setMinimum(-40.0);
  this->referenceLevelSpinBox->setMaximum(100.0);
  this->referenceLevelSpinBox->setValue(
      this->settings->scope.spectrumReference);
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
DsoConfigAnalysisPage::~DsoConfigAnalysisPage() {}

/// \brief Saves the new settings.
void DsoConfigAnalysisPage::saveSettings() {
  this->settings->scope.spectrumWindow =
      (Dso::WindowFunction)this->windowFunctionComboBox->currentIndex();
  this->settings->scope.spectrumReference =
      this->referenceLevelSpinBox->value();
  this->settings->scope.spectrumLimit = this->minimumMagnitudeSpinBox->value();
}

////////////////////////////////////////////////////////////////////////////////
// class DsoConfigColorsPage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigColorsPage::DsoConfigColorsPage(DsoSettings *settings, QWidget *parent)
    : QWidget(parent) {
  this->settings = settings;

  // Initialize elements
  DsoSettingsViewColor& colorSettings = this->settings->view.color;
  enum {
    COL_LABEL = 0,
    COL_SCR_CHANNEL,
    COL_SCR_SPECTRUM,
    COL_PRT_CHANNEL,
    COL_PRT_SPECTRUM
  };

  // Plot Area
  this->graphLabel = new QLabel(tr("<hr width=\"100%\"/>")); // 4*80
  this->graphLabel->setAlignment(Qt::AlignRight);
  this->graphLabel->setTextFormat(Qt::RichText);

  this->screenColorsLabel = new QLabel(tr("Screen"));
  this->screenColorsLabel->setAlignment(Qt::AlignHCenter);
  this->printColorsLabel = new QLabel(tr("Print"));
  this->printColorsLabel->setAlignment(Qt::AlignHCenter);

  this->axesLabel = new QLabel(tr("Axes"));
  this->axesColorBox = new ColorBox(colorSettings.screen.axes);
  this->printAxesColorBox = new ColorBox(colorSettings.print.axes);

  this->backgroundLabel = new QLabel(tr("Background"));
  this->backgroundColorBox = new ColorBox(colorSettings.screen.background);
  this->printBackgroundColorBox =
      new ColorBox(colorSettings.print.background);

  this->borderLabel = new QLabel(tr("Border"));
  this->borderColorBox = new ColorBox(colorSettings.screen.border);
  this->printBorderColorBox = new ColorBox(colorSettings.print.border);

  this->gridLabel = new QLabel(tr("Grid"));
  this->gridColorBox = new ColorBox(colorSettings.screen.grid);
  this->printGridColorBox = new ColorBox(colorSettings.print.grid);

  this->markersLabel = new QLabel(tr("Markers"));
  this->markersColorBox = new ColorBox(colorSettings.screen.markers);
  this->printMarkersColorBox =
      new ColorBox(colorSettings.print.markers);

  this->textLabel = new QLabel(tr("Text"));
  this->textColorBox = new ColorBox(colorSettings.screen.text);
  this->printTextColorBox = new ColorBox(colorSettings.print.text);

  // Graph category
  this->screenChannelLabel = new QLabel(tr("Channel"));
  this->screenChannelLabel->setAlignment(Qt::AlignHCenter);
  this->screenSpectrumLabel = new QLabel(tr("Spectrum"));
  this->screenSpectrumLabel->setAlignment(Qt::AlignHCenter);
  this->printChannelLabel = new QLabel(tr("Channel"));
  this->printChannelLabel->setAlignment(Qt::AlignHCenter);
  this->printSpectrumLabel = new QLabel(tr("Spectrum"));
  this->printSpectrumLabel->setAlignment(Qt::AlignHCenter);

  for (int channel = 0; channel < this->settings->scope.voltage.count();
       ++channel) {
    this->colorLabel.append(
        new QLabel(this->settings->scope.voltage[channel].name));
    this->screenChannelColorBox.append(
        new ColorBox(colorSettings.screen.voltage[channel]));
    this->screenSpectrumColorBox.append(
        new ColorBox(colorSettings.screen.spectrum[channel]));
    this->printChannelColorBox.append(
        new ColorBox(colorSettings.print.voltage[channel]));
    this->printSpectrumColorBox.append(
        new ColorBox(colorSettings.print.spectrum[channel]));
  }

  // Plot Area Layout
  this->colorsLayout = new QGridLayout();
  this->colorsLayout->setColumnStretch(COL_LABEL, 1);
  this->colorsLayout->setColumnMinimumWidth(COL_SCR_CHANNEL, 80);
  this->colorsLayout->setColumnMinimumWidth(COL_SCR_SPECTRUM, 80);
  this->colorsLayout->setColumnMinimumWidth(COL_PRT_CHANNEL, 80);
  this->colorsLayout->setColumnMinimumWidth(COL_PRT_SPECTRUM, 80);

  int row = 0;
  this->colorsLayout->addWidget(this->screenColorsLabel, row,
                                COL_SCR_CHANNEL, 1, 2);
  this->colorsLayout->addWidget(this->printColorsLabel, row,
                                COL_PRT_CHANNEL, 1, 2);
  ++row;
  this->colorsLayout->addWidget(this->backgroundLabel, row, COL_LABEL);
  this->colorsLayout->addWidget(this->backgroundColorBox,
                                row, COL_SCR_CHANNEL, 1, 2);
  this->colorsLayout->addWidget(this->printBackgroundColorBox,
                                row, COL_PRT_CHANNEL, 1, 2);
  ++row;
  this->colorsLayout->addWidget(this->gridLabel, row, COL_LABEL);
  this->colorsLayout->addWidget(this->gridColorBox,
                                row, COL_SCR_CHANNEL, 1, 2);
  this->colorsLayout->addWidget(this->printGridColorBox,
                                row, COL_PRT_CHANNEL, 1, 2);
  ++row;
  this->colorsLayout->addWidget(this->axesLabel, row, COL_LABEL);
  this->colorsLayout->addWidget(this->axesColorBox,
                                row, COL_SCR_CHANNEL, 1, 2);
  this->colorsLayout->addWidget(this->printAxesColorBox,
                                row, COL_PRT_CHANNEL, 1, 2);
  ++row;
  this->colorsLayout->addWidget(this->borderLabel, row, COL_LABEL);
  this->colorsLayout->addWidget(this->borderColorBox,
                                row, COL_SCR_CHANNEL, 1, 2);
  this->colorsLayout->addWidget(this->printBorderColorBox,
                                row, COL_PRT_CHANNEL, 1, 2);
  ++row;
  this->colorsLayout->addWidget(this->markersLabel, row, COL_LABEL);
  this->colorsLayout->addWidget(this->markersColorBox,
                                row, COL_SCR_CHANNEL, 1, 2);
  this->colorsLayout->addWidget(this->printMarkersColorBox,
                                row, COL_PRT_CHANNEL, 1, 2);
  ++row;
  this->colorsLayout->addWidget(this->textLabel, row, COL_LABEL);
  this->colorsLayout->addWidget(this->textColorBox,
                                row, COL_SCR_CHANNEL, 1, 2);
  this->colorsLayout->addWidget(this->printTextColorBox,
                                row, COL_PRT_CHANNEL, 1, 2);
  ++row;

  // Graph
  this->colorsLayout->addWidget(this->graphLabel, row, COL_LABEL, 1,
                                COL_PRT_SPECTRUM - COL_LABEL + 1);
  ++row;

  this->colorsLayout->addWidget(this->screenChannelLabel, row,
                                COL_SCR_CHANNEL);
  this->colorsLayout->addWidget(this->screenSpectrumLabel, row,
                                COL_SCR_SPECTRUM);
  this->colorsLayout->addWidget(this->printChannelLabel, row,
                                COL_PRT_CHANNEL);
  this->colorsLayout->addWidget(this->printSpectrumLabel, row,
                                COL_PRT_SPECTRUM);
  ++row;

  for (int channel = 0; channel < this->settings->scope.voltage.count();
       ++channel, ++row) {
    this->colorsLayout->addWidget(this->colorLabel[channel],
                                  row, COL_LABEL);
    this->colorsLayout->addWidget(this->screenChannelColorBox[channel],
                                  row, COL_SCR_CHANNEL);
    this->colorsLayout->addWidget(this->screenSpectrumColorBox[channel],
                                  row, COL_SCR_SPECTRUM);
    this->colorsLayout->addWidget(this->printChannelColorBox[channel],
                                  row, COL_PRT_CHANNEL);
    this->colorsLayout->addWidget(this->printSpectrumColorBox[channel],
                                  row, COL_PRT_SPECTRUM);
  }

  this->colorsGroup = new QGroupBox(tr("Screen and Print Colors"));
  this->colorsGroup->setLayout(this->colorsLayout);

  // Main layout
  this->mainLayout = new QVBoxLayout();
  this->mainLayout->addWidget(this->colorsGroup);
  this->mainLayout->addStretch(1);

  this->setLayout(this->mainLayout);
}

/// \brief Cleans up the widget.
DsoConfigColorsPage::~DsoConfigColorsPage() {}

/// \brief Saves the new settings.
void DsoConfigColorsPage::saveSettings() {

  DsoSettingsViewColor& colorSettings = this->settings->view.color;

  // Screen category
  colorSettings.screen.axes = this->axesColorBox->getColor();
  colorSettings.screen.background = this->backgroundColorBox->getColor();
  colorSettings.screen.border = this->borderColorBox->getColor();
  colorSettings.screen.grid = this->gridColorBox->getColor();
  colorSettings.screen.markers = this->markersColorBox->getColor();
  colorSettings.screen.text = this->textColorBox->getColor();

  // Print category
  colorSettings.print.axes = this->printAxesColorBox->getColor();
  colorSettings.print.background = this->printBackgroundColorBox->getColor();
  colorSettings.print.border = this->printBorderColorBox->getColor();
  colorSettings.print.grid = this->printGridColorBox->getColor();
  colorSettings.print.markers = this->printMarkersColorBox->getColor();
  colorSettings.print.text = this->printTextColorBox->getColor();

  // Graph category
  for (int channel = 0; channel < this->settings->scope.voltage.count();
       ++channel) {
    colorSettings.screen.voltage[channel] =
        this->screenChannelColorBox[channel]->getColor();
    colorSettings.screen.spectrum[channel] =
        this->screenSpectrumColorBox[channel]->getColor();
    colorSettings.print.voltage[channel] =
        this->printChannelColorBox[channel]->getColor();
    colorSettings.print.spectrum[channel] =
        this->printSpectrumColorBox[channel]->getColor();
  }
}

////////////////////////////////////////////////////////////////////////////////
// class DsoConfigFilesPage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigFilesPage::DsoConfigFilesPage(DsoSettings *settings, QWidget *parent)
    : QWidget(parent) {
  this->settings = settings;

  // Initialize elements

  // Export group
  this->screenColorCheckBox = new QCheckBox(tr("Export Images with Screen Colors"));
  this->screenColorCheckBox->setChecked(this->settings->view.screenColorImages);

  this->imageWidthLabel = new QLabel(tr("Image width"));
  this->imageWidthSpinBox = new QSpinBox();
  this->imageWidthSpinBox->setMinimum(100);
  this->imageWidthSpinBox->setMaximum(9999);
  this->imageWidthSpinBox->setValue(this->settings->options.imageSize.width());
  this->imageHeightLabel = new QLabel(tr("Image height"));
  this->imageHeightSpinBox = new QSpinBox();
  this->imageHeightSpinBox->setMinimum(100);
  this->imageHeightSpinBox->setMaximum(9999);
  this->imageHeightSpinBox->setValue(
      this->settings->options.imageSize.height());

  this->exportLayout = new QGridLayout();
  this->exportLayout->addWidget(this->screenColorCheckBox, 0, 0, 1, 2);
  this->exportLayout->addWidget(this->imageWidthLabel, 1, 0);
  this->exportLayout->addWidget(this->imageWidthSpinBox, 1, 1);
  this->exportLayout->addWidget(this->imageHeightLabel, 2, 0);
  this->exportLayout->addWidget(this->imageHeightSpinBox, 2, 1);

  this->exportGroup = new QGroupBox(tr("Export"));
  this->exportGroup->setLayout(this->exportLayout);

  // Configuration group
  this->saveOnExitCheckBox = new QCheckBox(tr("Save default settings on exit"));
  this->saveOnExitCheckBox->setChecked(this->settings->options.alwaysSave);
  this->saveNowButton = new QPushButton(tr("Save default settings now"));

  this->configurationLayout = new QVBoxLayout();
  this->configurationLayout->addWidget(this->saveOnExitCheckBox, 0);
  this->configurationLayout->addWidget(this->saveNowButton, 1);

  this->configurationGroup = new QGroupBox(tr("Configuration"));
  this->configurationGroup->setLayout(this->configurationLayout);

  // Main layout
  this->mainLayout = new QVBoxLayout();
  this->mainLayout->addWidget(this->exportGroup);
  this->mainLayout->addWidget(this->configurationGroup);
  this->mainLayout->addStretch(1);

  this->setLayout(this->mainLayout);

  connect(this->saveNowButton, SIGNAL(clicked()), this->settings, SLOT(save()));
}

/// \brief Cleans up the widget.
DsoConfigFilesPage::~DsoConfigFilesPage() {}

/// \brief Saves the new settings.
void DsoConfigFilesPage::saveSettings() {
  this->settings->options.alwaysSave = this->saveOnExitCheckBox->isChecked();
  this->settings->view.screenColorImages = this->screenColorCheckBox->isChecked();
  this->settings->options.imageSize.setWidth(this->imageWidthSpinBox->value());
  this->settings->options.imageSize.setHeight(
      this->imageHeightSpinBox->value());
}

////////////////////////////////////////////////////////////////////////////////
// class DsoConfigScopePage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigScopePage::DsoConfigScopePage(DsoSettings *settings, QWidget *parent)
    : QWidget(parent) {
  this->settings = settings;

  // Initialize lists for comboboxes
  QStringList interpolationStrings;
  interpolationStrings << tr("Off") << tr("Linear") << tr("Sinc");

  // Initialize elements
  this->antialiasingCheckBox = new QCheckBox(tr("Antialiasing"));
  this->antialiasingCheckBox->setChecked(this->settings->view.antialiasing);
  this->interpolationLabel = new QLabel(tr("Interpolation"));
  this->interpolationComboBox = new QComboBox();
  this->interpolationComboBox->addItems(interpolationStrings);
  this->interpolationComboBox->setCurrentIndex(
      this->settings->view.interpolation);
  this->digitalPhosphorDepthLabel = new QLabel(tr("Digital phosphor depth"));
  this->digitalPhosphorDepthSpinBox = new QSpinBox();
  this->digitalPhosphorDepthSpinBox->setMinimum(2);
  this->digitalPhosphorDepthSpinBox->setMaximum(99);
  this->digitalPhosphorDepthSpinBox->setValue(
      this->settings->view.digitalPhosphorDepth);

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
DsoConfigScopePage::~DsoConfigScopePage() {}

/// \brief Saves the new settings.
void DsoConfigScopePage::saveSettings() {
  this->settings->view.antialiasing = this->antialiasingCheckBox->isChecked();
  this->settings->view.interpolation =
      (Dso::InterpolationMode)this->interpolationComboBox->currentIndex();
  this->settings->view.digitalPhosphorDepth =
      this->digitalPhosphorDepthSpinBox->value();
}
