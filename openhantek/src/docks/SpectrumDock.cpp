// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "SpectrumDock.h"
#include "dockwindows.h"

#include "settings.h"
#include "sispinbox.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"

////////////////////////////////////////////////////////////////////////////////
// class SpectrumDock
/// \brief Initializes the spectrum view docking window.
/// \param settings The target settings object.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
SpectrumDock::SpectrumDock(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(tr("Spectrum"), parent, flags), settings(settings) {

    // Initialize lists for comboboxes
    this->magnitudeSteps << 1e0 << 2e0 << 3e0 << 6e0 << 1e1 << 2e1 << 3e1 << 6e1 << 1e2 << 2e2 << 3e2
                         << 6e2; ///< Magnitude steps in dB/div
    for (QList<double>::iterator magnitude = this->magnitudeSteps.begin(); magnitude != this->magnitudeSteps.end();
         ++magnitude)
        this->magnitudeStrings << valueToString(*magnitude, UNIT_DECIBEL, 0);

    // Initialize elements
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        this->magnitudeComboBox.append(new QComboBox());
        this->magnitudeComboBox[channel]->addItems(this->magnitudeStrings);

        this->usedCheckBox.append(new QCheckBox(settings->scope.voltage[channel].name));
    }

    this->dockLayout = new QGridLayout();
    this->dockLayout->setColumnMinimumWidth(0, 64);
    this->dockLayout->setColumnStretch(1, 1);
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        this->dockLayout->addWidget(this->usedCheckBox[channel], channel, 0);
        this->dockLayout->addWidget(this->magnitudeComboBox[channel], channel, 1);
    }

    this->dockWidget = new QWidget();
    SetupDockWidget(this, dockWidget, dockLayout);

    // Connect signals and slots
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        connect(this->magnitudeComboBox[channel], SIGNAL(currentIndexChanged(int)), this, SLOT(magnitudeSelected(int)));
        connect(this->usedCheckBox[channel], SIGNAL(toggled(bool)), this, SLOT(usedSwitched(bool)));
    }

    // Set values
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        this->setMagnitude(channel, settings->scope.spectrum[channel].magnitude);
        this->setUsed(channel, settings->scope.spectrum[channel].used);
    }
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void SpectrumDock::closeEvent(QCloseEvent *event) {
    this->hide();

    event->accept();
}

/// \brief Sets the magnitude for a channel.
/// \param channel The channel, whose magnitude should be set.
/// \param magnitude The magnitude in dB.
/// \return Index of magnitude-value, -1 on error.
int SpectrumDock::setMagnitude(int channel, double magnitude) {
    if (channel < 0 || channel >= settings->scope.voltage.count()) return -1;

    int index = this->magnitudeSteps.indexOf(magnitude);
    if (index != -1) this->magnitudeComboBox[channel]->setCurrentIndex(index);

    return index;
}

/// \brief Enables/disables a channel.
/// \param channel The channel, that should be enabled/disabled.
/// \param used True if the channel should be enabled, false otherwise.
/// \return Index of channel, -1 on error.
int SpectrumDock::setUsed(int channel, bool used) {
    if (channel >= 0 && channel < settings->scope.voltage.count()) {
        this->usedCheckBox[channel]->setChecked(used);
        return channel;
    }

    return -1;
}

/// \brief Called when the source combo box changes it's value.
/// \param index The index of the combo box item.
void SpectrumDock::magnitudeSelected(int index) {
    int channel;

    // Which combobox was it?
    for (channel = 0; channel < settings->scope.voltage.count(); ++channel)
        if (this->sender() == this->magnitudeComboBox[channel]) break;

    // Send signal if it was one of the comboboxes
    if (channel < settings->scope.voltage.count()) {
        settings->scope.spectrum[channel].magnitude = this->magnitudeSteps.at(index);
        emit magnitudeChanged(channel, settings->scope.spectrum[channel].magnitude);
    }
}

/// \brief Called when the used checkbox is switched.
/// \param checked The check-state of the checkbox.
void SpectrumDock::usedSwitched(bool checked) {
    int channel;

    // Which checkbox was it?
    for (channel = 0; channel < settings->scope.voltage.count(); ++channel)
        if (this->sender() == this->usedCheckBox[channel]) break;

    // Send signal if it was one of the checkboxes
    if (channel < settings->scope.voltage.count()) {
        settings->scope.spectrum[channel].used = checked;
        emit usedChanged(channel, checked);
    }
}
