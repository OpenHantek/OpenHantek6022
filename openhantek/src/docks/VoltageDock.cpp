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
    for (Dso::Coupling c: settings->deviceSpecification->couplings)
        couplingStrings.append(Dso::couplingString(c));

    for( auto e: Dso::MathModeEnum ) {
        modeStrings.append(Dso::mathModeString(e));
    }

    for (double gainStep: settings->scope.gainSteps)
        gainStrings << valueToString(gainStep, UNIT_VOLTS, 0);

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth(0, 64);
    dockLayout->setColumnStretch(1, 1);

    // Initialize elements
    for (ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel) {
        ChannelBlock b;

        b.miscComboBox=(new QComboBox());
        b.gainComboBox=(new QComboBox());
        b.invertCheckBox=(new QCheckBox(tr("Invert")));
        b.usedCheckBox=(new QCheckBox(settings->scope.voltage[channel].name));

        if (channel < settings->deviceSpecification->channels)
            b.miscComboBox->addItems(couplingStrings);
        else
            b.miscComboBox->addItems(modeStrings);

        b.gainComboBox->addItems(gainStrings);

        dockLayout->addWidget(b.usedCheckBox, (int)channel * 3, 0);
        dockLayout->addWidget(b.gainComboBox, (int)channel * 3, 1);
        dockLayout->addWidget(b.miscComboBox, (int)channel * 3 + 1, 1);
        dockLayout->addWidget(b.invertCheckBox, (int)channel * 3 + 2, 1);

        connect(b.gainComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this,channel](int index) {
            this->settings->scope.voltage[channel].gainStepIndex = (unsigned)index;
            emit gainChanged(channel, this->settings->scope.gain(channel));
        });
        connect(b.invertCheckBox, &QAbstractButton::toggled, [this,channel](bool checked) {
            this->settings->scope.voltage[channel].inverted = checked;
        });
        connect(b.miscComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this,channel](int index){
            if (channel < this->settings->deviceSpecification->channels) {
                this->settings->scope.voltage[channel].couplingIndex = (unsigned)index;
                emit couplingChanged(channel, this->settings->coupling(channel));
            } else {
                this->settings->scope.voltage[channel].math = (Dso::MathMode) index;
                emit modeChanged(this->settings->scope.voltage[channel].math);
            }
        });
        connect(b.usedCheckBox, &QAbstractButton::toggled, [this,channel](bool checked) {
            this->settings->scope.voltage[channel].used = checked;
            emit usedChanged(channel, checked);
        });

        channelBlocks.push_back(std::move(b));

        if (channel < settings->deviceSpecification->channels)
            setCoupling(channel, settings->scope.voltage[channel].couplingIndex);
        else
            setMode(settings->scope.voltage[channel].math);
        setGain(channel, settings->scope.voltage[channel].gainStepIndex);
        setUsed(channel, settings->scope.voltage[channel].used);
    }

    dockWidget = new QWidget();
    SetupDockWidget(this, dockWidget, dockLayout);
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void VoltageDock::closeEvent(QCloseEvent *event) {
    hide();
    event->accept();
}

void VoltageDock::setCoupling(ChannelID channel, unsigned couplingIndex) {
    if (channel >= settings->deviceSpecification->channels) return;
    if (couplingIndex >= settings->deviceSpecification->couplings.size()) return;
    channelBlocks[channel].miscComboBox->setCurrentIndex((int)couplingIndex);
}

void VoltageDock::setGain(ChannelID channel, unsigned gainStepIndex) {
    if (channel >= settings->scope.voltage.size()) return;
    if (gainStepIndex >= settings->scope.gainSteps.size()) return;
    channelBlocks[channel].gainComboBox->setCurrentIndex((unsigned)gainStepIndex);
}

void VoltageDock::setMode(Dso::MathMode mode) {
    channelBlocks[settings->deviceSpecification->channels].miscComboBox->setCurrentIndex((int)mode);
}

void VoltageDock::setUsed(ChannelID channel, bool used) {
    if (channel >= settings->scope.voltage.size()) return;
    channelBlocks[channel].usedCheckBox->setChecked(used);
}
