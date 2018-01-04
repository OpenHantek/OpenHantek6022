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
    this->magnitudeSteps = { 1e0 , 2e0 , 3e0 , 6e0 , 1e1 , 2e1 , 3e1 , 6e1 , 1e2 , 2e2 , 3e2, 6e2 };
    for (const auto& magnitude: magnitudeSteps)
        this->magnitudeStrings << valueToString(magnitude, UNIT_DECIBEL, 0);

    // Initialize elements
    for (ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel) {
        this->magnitudeComboBox.push_back(new QComboBox());
        this->magnitudeComboBox[channel]->addItems(this->magnitudeStrings);

        this->usedCheckBox.push_back(new QCheckBox(settings->scope.voltage[channel].name));
    }

    this->dockLayout = new QGridLayout();
    this->dockLayout->setColumnMinimumWidth(0, 64);
    this->dockLayout->setColumnStretch(1, 1);
    for (ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel) {
        this->dockLayout->addWidget(this->usedCheckBox[channel], (int)channel, 0);
        this->dockLayout->addWidget(this->magnitudeComboBox[channel], (int)channel, 1);

        // Connect signals and slots
        connect(usedCheckBox[channel], &QCheckBox::toggled, this, &SpectrumDock::usedSwitched);
        QComboBox* cmb = magnitudeComboBox[channel];
        connect(cmb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpectrumDock::magnitudeSelected);

        // Set values
        this->setMagnitude(channel, settings->scope.spectrum[channel].magnitude);
        this->setUsed(channel, settings->scope.spectrum[channel].used);
    }

    dockWidget = new QWidget();
    SetupDockWidget(this, dockWidget, dockLayout);
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
int SpectrumDock::setMagnitude(ChannelID channel, double magnitude) {
    if (channel >= settings->scope.voltage.size()) return -1;

    auto indexIt = std::find(magnitudeSteps.begin(),magnitudeSteps.end(),magnitude);
    if (indexIt == magnitudeSteps.end()) return -1;
    int index = (int)std::distance(magnitudeSteps.begin(), indexIt);
    magnitudeComboBox[channel]->setCurrentIndex(index);
    return index;
}

/// \brief Enables/disables a channel.
/// \param channel The channel, that should be enabled/disabled.
/// \param used True if the channel should be enabled, false otherwise.
/// \return Index of channel, INT_MAX on error.
unsigned SpectrumDock::setUsed(ChannelID channel, bool used) {
    if (channel >= settings->scope.voltage.size()) return INT_MAX;

    this->usedCheckBox[channel]->setChecked(used);
    return channel;
}

/// \brief Called when the source combo box changes it's value.
/// \param index The index of the combo box item.
void SpectrumDock::magnitudeSelected(unsigned index) {
    ChannelID channel;

    // Which combobox was it?
    for (channel = 0; channel < settings->scope.voltage.size(); ++channel)
        if (this->sender() == this->magnitudeComboBox[channel]) break;

    // Send signal if it was one of the comboboxes
    if (channel < settings->scope.voltage.size()) {
        settings->scope.spectrum[channel].magnitude = this->magnitudeSteps.at(index);
        emit magnitudeChanged(channel, settings->scope.spectrum[channel].magnitude);
    }
}

/// \brief Called when the used checkbox is switched.
/// \param checked The check-state of the checkbox.
void SpectrumDock::usedSwitched(bool checked) {
    ChannelID channel;

    // Which checkbox was it?
    for (channel = 0; channel < settings->scope.voltage.size(); ++channel)
        if (this->sender() == this->usedCheckBox[channel]) break;

    // Send signal if it was one of the checkboxes
    if (channel < settings->scope.voltage.size()) {
        settings->scope.spectrum[channel].used = checked;
        emit usedChanged(channel, checked);
    }
}
