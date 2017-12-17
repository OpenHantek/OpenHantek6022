// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "dockwindows.h"
#include "HorizontalDock.h"

#include "utils/printutils.h"
#include "utils/dsoStrings.h"
#include "settings.h"
#include "sispinbox.h"

////////////////////////////////////////////////////////////////////////////////
// class HorizontalDock
/// \brief Initializes the horizontal axis docking window.
/// \param settings The target settings object.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
HorizontalDock::HorizontalDock(DsoSettings *settings, QWidget *parent,
                               Qt::WindowFlags flags)
    : QDockWidget(tr("Horizontal"), parent, flags) {
  this->settings = settings;

  // Initialize elements
  this->samplerateLabel = new QLabel(tr("Samplerate"));
  this->samplerateSiSpinBox = new SiSpinBox(UNIT_SAMPLES);
  this->samplerateSiSpinBox->setMinimum(1);
  this->samplerateSiSpinBox->setMaximum(1e8);
  this->samplerateSiSpinBox->setUnitPostfix("/s");

  timebaseSteps << 1.0 << 2.0 << 4.0 << 10.0;

  this->timebaseLabel = new QLabel(tr("Timebase"));
  this->timebaseSiSpinBox = new SiSpinBox(UNIT_SECONDS);
  this->timebaseSiSpinBox->setSteps(timebaseSteps);
  this->timebaseSiSpinBox->setMinimum(1e-9);
  this->timebaseSiSpinBox->setMaximum(3.6e3);

  this->frequencybaseLabel = new QLabel(tr("Frequencybase"));
  this->frequencybaseSiSpinBox = new SiSpinBox(UNIT_HERTZ);
  this->frequencybaseSiSpinBox->setMinimum(1.0);
  this->frequencybaseSiSpinBox->setMaximum(100e6);

  this->recordLengthLabel = new QLabel(tr("Record length"));
  this->recordLengthComboBox = new QComboBox();

  this->formatLabel = new QLabel(tr("Format"));
  this->formatComboBox = new QComboBox();
  for (int format = Dso::GRAPHFORMAT_TY; format < Dso::GRAPHFORMAT_COUNT;
       ++format)
    this->formatComboBox->addItem(
        Dso::graphFormatString((Dso::GraphFormat)format));

  this->dockLayout = new QGridLayout();
  this->dockLayout->setColumnMinimumWidth(0, 64);
  this->dockLayout->setColumnStretch(1, 1);
  this->dockLayout->addWidget(this->samplerateLabel, 0, 0);
  this->dockLayout->addWidget(this->samplerateSiSpinBox, 0, 1);
  this->dockLayout->addWidget(this->timebaseLabel, 1, 0);
  this->dockLayout->addWidget(this->timebaseSiSpinBox, 1, 1);
  this->dockLayout->addWidget(this->frequencybaseLabel, 2, 0);
  this->dockLayout->addWidget(this->frequencybaseSiSpinBox, 2, 1);
  this->dockLayout->addWidget(this->recordLengthLabel, 3, 0);
  this->dockLayout->addWidget(this->recordLengthComboBox, 3, 1);
  this->dockLayout->addWidget(this->formatLabel, 4, 0);
  this->dockLayout->addWidget(this->formatComboBox, 4, 1);

  this->dockWidget = new QWidget();
  SetupDockWidget(this, dockWidget, dockLayout);

  // Connect signals and slots
  connect(this->samplerateSiSpinBox, SIGNAL(valueChanged(double)), this,
          SLOT(samplerateSelected(double)));
  connect(this->timebaseSiSpinBox, SIGNAL(valueChanged(double)), this,
          SLOT(timebaseSelected(double)));
  connect(this->frequencybaseSiSpinBox, SIGNAL(valueChanged(double)), this,
          SLOT(frequencybaseSelected(double)));
  connect(this->recordLengthComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(recordLengthSelected(int)));
  connect(this->formatComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(formatSelected(int)));

  // Set values
  this->setSamplerate(this->settings->scope.horizontal.samplerate);
  this->setTimebase(this->settings->scope.horizontal.timebase);
  this->setFrequencybase(this->settings->scope.horizontal.frequencybase);
  this->setRecordLength(this->settings->scope.horizontal.recordLength);
  this->setFormat(this->settings->scope.horizontal.format);
}

/// \brief Cleans up everything.
HorizontalDock::~HorizontalDock() {}

/// \brief Don't close the dock, just hide it.
/// \param event The close event that should be handled.
void HorizontalDock::closeEvent(QCloseEvent *event) {
  this->hide();

  event->accept();
}

/// \brief Changes the frequencybase.
/// \param frequencybase The frequencybase in hertz.
void HorizontalDock::setFrequencybase(double frequencybase) {
  this->suppressSignals = true;
  this->frequencybaseSiSpinBox->setValue(frequencybase);
  this->suppressSignals = false;
}

/// \brief Changes the samplerate.
/// \param samplerate The samplerate in seconds.
void HorizontalDock::setSamplerate(double samplerate) {
  this->suppressSignals = true;
  this->samplerateSiSpinBox->setValue(samplerate);
  this->suppressSignals = false;
}

/// \brief Changes the timebase.
/// \param timebase The timebase in seconds.
double HorizontalDock::setTimebase(double timebase) {
  // timebaseSteps are repeated in each decade
  double decade = pow(10, floor(log10(timebase)));
  double vNorm = timebase / decade;
  for (int i = 0; i < timebaseSteps.size() - 1; ++i) {
    if (timebaseSteps.at(i) <= vNorm && vNorm < timebaseSteps.at(i + 1)) {
      suppressSignals = true;
      timebaseSiSpinBox->setValue(decade * timebaseSteps.at(i));
      suppressSignals = false;
      break;
    }
  }
  return timebaseSiSpinBox->value();
}

/// \brief Changes the record length if the new value is supported.
/// \param recordLength The record length in samples.
void HorizontalDock::setRecordLength(unsigned int recordLength) {
  int index = this->recordLengthComboBox->findData(recordLength);

  if (index != -1) {
    this->suppressSignals = true;
    this->recordLengthComboBox->setCurrentIndex(index);
    this->suppressSignals = false;
  }
}

/// \brief Changes the format if the new value is supported.
/// \param format The format for the horizontal axis.
/// \return Index of format-value, -1 on error.
int HorizontalDock::setFormat(Dso::GraphFormat format) {
  if (format >= Dso::GRAPHFORMAT_TY && format <= Dso::GRAPHFORMAT_XY) {
    this->suppressSignals = true;
    this->formatComboBox->setCurrentIndex(format);
    this->suppressSignals = false;
    return format;
  }

  return -1;
}

/// \brief Updates the available record lengths in the combo box.
/// \param recordLengths The available record lengths for the combo box.
void HorizontalDock::availableRecordLengthsChanged(
    const QList<unsigned int> &recordLengths) {
  /// \todo Empty lists should be interpreted as scope supporting continuous
  /// record length values.
  this->recordLengthComboBox->blockSignals(
      true); // Avoid messing up the settings
  this->recordLengthComboBox->setUpdatesEnabled(false);

  // Update existing elements to avoid unnecessary index updates
  int index = 0;
  for (; index < recordLengths.size(); ++index) {
    unsigned int recordLengthItem = recordLengths[index];
    if (index < this->recordLengthComboBox->count()) {
      this->recordLengthComboBox->setItemData(index, recordLengthItem);
      this->recordLengthComboBox->setItemText(
          index, recordLengthItem == UINT_MAX
                     ? tr("Roll")
                     : valueToString(recordLengthItem,
                                             UNIT_SAMPLES, 3));
    } else {
      this->recordLengthComboBox->addItem(
          recordLengthItem == UINT_MAX
              ? tr("Roll")
              : valueToString(recordLengthItem, UNIT_SAMPLES,
                                      3),
          (uint)recordLengthItem);
    }
  }
  // Remove extra elements
  for (int extraIndex = this->recordLengthComboBox->count() - 1;
       extraIndex > index; --extraIndex) {
    this->recordLengthComboBox->removeItem(extraIndex);
  }

  this->setRecordLength(this->settings->scope.horizontal.recordLength);
  this->recordLengthComboBox->setUpdatesEnabled(true);
  this->recordLengthComboBox->blockSignals(false);
}

/// \brief Updates the minimum and maximum of the samplerate spin box.
/// \param minimum The minimum value the spin box should accept.
/// \param maximum The minimum value the spin box should accept.
void HorizontalDock::samplerateLimitsChanged(double minimum, double maximum) {
  this->suppressSignals = true;
  this->samplerateSiSpinBox->setMinimum(minimum);
  this->samplerateSiSpinBox->setMaximum(maximum);
  this->suppressSignals = false;
}

/// \brief Updates the mode and steps of the samplerate spin box.
/// \param mode The mode value the spin box should accept.
/// \param steps The steps value the spin box should accept.
void HorizontalDock::samplerateSet(int mode, QList<double> steps) {
  this->suppressSignals = true;
  this->samplerateSiSpinBox->setMode(mode);
  this->samplerateSiSpinBox->setSteps(steps);
  this->suppressSignals = false;
}

/// \brief Called when the frequencybase spinbox changes its value.
/// \param frequencybase The frequencybase in hertz.
void HorizontalDock::frequencybaseSelected(double frequencybase) {
  this->settings->scope.horizontal.frequencybase = frequencybase;
  if (!this->suppressSignals)
    emit frequencybaseChanged(frequencybase);
}

/// \brief Called when the samplerate spinbox changes its value.
/// \param samplerate The samplerate in samples/second.
void HorizontalDock::samplerateSelected(double samplerate) {
  this->settings->scope.horizontal.samplerate = samplerate;
  if (!this->suppressSignals) {
    this->settings->scope.horizontal.samplerateSet = true;
    emit samplerateChanged(samplerate);
  }
}

/// \brief Called when the timebase spinbox changes its value.
/// \param timebase The timebase in seconds.
void HorizontalDock::timebaseSelected(double timebase) {
  this->settings->scope.horizontal.timebase = timebase;
  if (!this->suppressSignals) {
    this->settings->scope.horizontal.samplerateSet = false;
    emit timebaseChanged(timebase);
  }
}

/// \brief Called when the record length combo box changes its value.
/// \param index The index of the combo box item.
void HorizontalDock::recordLengthSelected(int index) {
  this->settings->scope.horizontal.recordLength =
      this->recordLengthComboBox->itemData(index).toUInt();
  if (!this->suppressSignals)
    emit recordLengthChanged(index);
}

/// \brief Called when the format combo box changes its value.
/// \param index The index of the combo box item.
void HorizontalDock::formatSelected(int index) {
  this->settings->scope.horizontal.format = (Dso::GraphFormat)index;
  if (!this->suppressSignals)
    emit formatChanged(this->settings->scope.horizontal.format);
}
