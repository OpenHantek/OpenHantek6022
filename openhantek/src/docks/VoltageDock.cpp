// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "VoltageDock.h"
#include "dockwindows.h"

#include "settings.h"
#include "sispinbox.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"

////////////////////////////////////////////////////////////////////////////////
// class VoltageDock
/// \brief Initializes the vertical axis docking window.
/// \param settings The target settings object.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
VoltageDock::VoltageDock(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(tr("Voltage"), parent, flags), settings(settings) {

    // Initialize lists for comboboxes
    for (int coupling = Dso::COUPLING_AC; coupling < Dso::COUPLING_COUNT; ++coupling)
        this->couplingStrings.append(Dso::couplingString((Dso::Coupling)coupling));

    for (int mode = Dso::MATHMODE_1ADD2; mode < Dso::MATHMODE_COUNT; ++mode)
        this->modeStrings.append(Dso::mathModeString((Dso::MathMode)mode));

    this->gainSteps << 1e-2 << 2e-2 << 5e-2 << 1e-1 << 2e-1 << 5e-1 << 1e0 << 2e0 << 5e0; ///< Voltage steps in V/div
    for (QList<double>::iterator gain = this->gainSteps.begin(); gain != this->gainSteps.end(); ++gain)
        this->gainStrings << valueToString(*gain, UNIT_VOLTS, 0);

    // Initialize elements
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        this->miscComboBox.append(new QComboBox());
        if (channel < (int)settings->scope.physicalChannels)
            this->miscComboBox[channel]->addItems(this->couplingStrings);
        else
            this->miscComboBox[channel]->addItems(this->modeStrings);

        this->gainComboBox.append(new QComboBox());
        this->gainComboBox[channel]->addItems(this->gainStrings);

        this->invertCheckBox.append(new QCheckBox(tr("Invert")));

        this->usedCheckBox.append(new QCheckBox(settings->scope.voltage[channel].name));
    }

    this->dockLayout = new QGridLayout();
    this->dockLayout->setColumnMinimumWidth(0, 64);
    this->dockLayout->setColumnStretch(1, 1);
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        this->dockLayout->addWidget(this->usedCheckBox[channel], channel * 3, 0);
        this->dockLayout->addWidget(this->gainComboBox[channel], channel * 3, 1);
        this->dockLayout->addWidget(this->miscComboBox[channel], channel * 3 + 1, 1);
        this->dockLayout->addWidget(this->invertCheckBox[channel], channel * 3 + 2, 1);
    }

    this->dockWidget = new QWidget();
    SetupDockWidget(this, dockWidget, dockLayout);

    // Connect signals and slots
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        connect(this->gainComboBox[channel], SIGNAL(currentIndexChanged(int)), this, SLOT(gainSelected(int)));
        connect(this->invertCheckBox[channel], SIGNAL(toggled(bool)), this, SLOT(invertSwitched(bool)));
        connect(this->miscComboBox[channel], SIGNAL(currentIndexChanged(int)), this, SLOT(miscSelected(int)));
        connect(this->usedCheckBox[channel], SIGNAL(toggled(bool)), this, SLOT(usedSwitched(bool)));
    }

    // Set values
    for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
        if (channel < (int)settings->scope.physicalChannels)
            this->setCoupling(channel, (Dso::Coupling)settings->scope.voltage[channel].misc);
        else
            this->setMode((Dso::MathMode)settings->scope.voltage[channel].misc);
        this->setGain(channel, settings->scope.voltage[channel].gain);
        this->setUsed(channel, settings->scope.voltage[channel].used);
    }
}

/// \brief Cleans up everything.
VoltageDock::~VoltageDock() {}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void VoltageDock::closeEvent(QCloseEvent *event) {
    this->hide();

    event->accept();
}

/// \brief Sets the coupling for a channel.
/// \param channel The channel, whose coupling should be set.
/// \param coupling The coupling-mode.
/// \return Index of coupling-mode, -1 on error.
int VoltageDock::setCoupling(int channel, Dso::Coupling coupling) {
    if (coupling < Dso::COUPLING_AC || coupling > Dso::COUPLING_GND) return -1;
    if (channel < 0 || channel >= (int)settings->scope.physicalChannels) return -1;

    this->miscComboBox[channel]->setCurrentIndex(coupling);
    return coupling;
}

/// \brief Sets the gain for a channel.
/// \param channel The channel, whose gain should be set.
/// \param gain The gain in volts.
/// \return Index of gain-value, -1 on error.
int VoltageDock::setGain(int channel, double gain) {
    if (channel < 0 || channel >= settings->scope.voltage.count()) return -1;

    int index = this->gainSteps.indexOf(gain);
    if (index != -1) this->gainComboBox[channel]->setCurrentIndex(index);

    return index;
}

/// \brief Sets the mode for the math channel.
/// \param mode The math-mode.
/// \return Index of math-mode, -1 on error.
int VoltageDock::setMode(Dso::MathMode mode) {
    if (mode >= Dso::MATHMODE_1ADD2 && mode <= Dso::MATHMODE_2SUB1) {
        this->miscComboBox[settings->scope.physicalChannels]->setCurrentIndex(mode);
        return mode;
    }

    return -1;
}

/// \brief Enables/disables a channel.
/// \param channel The channel, that should be enabled/disabled.
/// \param used True if the channel should be enabled, false otherwise.
/// \return Index of channel, -1 on error.
int VoltageDock::setUsed(int channel, bool used) {
    if (channel >= 0 && channel < settings->scope.voltage.count()) {
        this->usedCheckBox[channel]->setChecked(used);
        return channel;
    }

    return -1;
}

/// \brief Called when the gain combo box changes it's value.
/// \param index The index of the combo box item.
void VoltageDock::gainSelected(int index) {
    int channel;

    // Which combobox was it?
    for (channel = 0; channel < settings->scope.voltage.count(); ++channel)
        if (this->sender() == this->gainComboBox[channel]) break;

    // Send signal if it was one of the comboboxes
    if (channel < settings->scope.voltage.count()) {
        settings->scope.voltage[channel].gain = this->gainSteps.at(index);

        emit gainChanged(channel, settings->scope.voltage[channel].gain);
    }
}

/// \brief Called when the misc combo box changes it's value.
/// \param index The index of the combo box item.
void VoltageDock::miscSelected(int index) {
    int channel;

    // Which combobox was it?
    for (channel = 0; channel < settings->scope.voltage.count(); ++channel)
        if (this->sender() == this->miscComboBox[channel]) break;

    // Send signal if it was one of the comboboxes
    if (channel < settings->scope.voltage.count()) {
        settings->scope.voltage[channel].misc = index;
        if (channel < (int)settings->scope.physicalChannels)
            emit couplingChanged(channel, (Dso::Coupling)settings->scope.voltage[channel].misc);
        else
            emit modeChanged((Dso::MathMode)settings->scope.voltage[channel].misc);
    }
}

/// \brief Called when the used checkbox is switched.
/// \param checked The check-state of the checkbox.
void VoltageDock::usedSwitched(bool checked) {
    int channel;

    // Which checkbox was it?
    for (channel = 0; channel < settings->scope.voltage.count(); ++channel)
        if (this->sender() == this->usedCheckBox[channel]) break;

    // Send signal if it was one of the checkboxes
    if (channel < settings->scope.voltage.count()) {
        settings->scope.voltage[channel].used = checked;
        emit usedChanged(channel, checked);
    }
}

/// \brief Called when the invert checkbox is switched.
/// \param checked The check-state of the checkbox.
void VoltageDock::invertSwitched(bool checked) {
    int channel;

    // Which checkbox was it?
    for (channel = 0; channel < settings->scope.voltage.count(); ++channel)
        if (this->sender() == this->invertCheckBox[channel]) break;

    // Send signal if it was one of the checkboxes
    if (channel < settings->scope.voltage.count()) {
        settings->scope.voltage[channel].inverted = checked;
        // Should we emit an event here?
    }
}
