// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "VoltageDock.h"
#include "dockwindows.h"

#include "settings.h"
#include "sispinbox.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"

template<typename... Args> struct SELECT {
    template<typename C, typename R>
    static constexpr auto OVERLOAD_OF( R (C::*pmf)(Args...) ) -> decltype(pmf) {
        return pmf;
    }
};

VoltageDock::VoltageDock(DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(tr("Voltage"), parent, flags), scope(scope), spec(spec) {

    // Initialize lists for comboboxes
    for (Dso::Coupling c: spec->couplings)
        couplingStrings.append(Dso::couplingString(c));

    for( auto e: Dso::MathModeEnum ) {
        modeStrings.append(Dso::mathModeString(e));
    }

    for (double gainStep: scope->gainSteps)
        gainStrings << valueToString(gainStep, UNIT_VOLTS, 0);

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth(0, 64);
    dockLayout->setColumnStretch(1, 1);

    // Initialize elements
    for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
        ChannelBlock b;

        b.miscComboBox=(new QComboBox());
        b.gainComboBox=(new QComboBox());
        b.invertCheckBox=(new QCheckBox(tr("Invert")));
        b.usedCheckBox=(new QCheckBox(scope->voltage[channel].name));

        channelBlocks.push_back(std::move(b));

        if (channel < spec->channels)
            b.miscComboBox->addItems(couplingStrings);
        else
            b.miscComboBox->addItems(modeStrings);

        b.gainComboBox->addItems(gainStrings);

        dockLayout->addWidget(b.usedCheckBox, (int)channel * 3, 0);
        dockLayout->addWidget(b.gainComboBox, (int)channel * 3, 1);
        dockLayout->addWidget(b.miscComboBox, (int)channel * 3 + 1, 1);
        dockLayout->addWidget(b.invertCheckBox, (int)channel * 3 + 2, 1);

        if (channel < spec->channels)
            setCoupling(channel, scope->voltage[channel].couplingIndex);
        else
            setMode(scope->voltage[channel].math);
        setGain(channel, scope->voltage[channel].gainStepIndex);
        setUsed(channel, scope->voltage[channel].used);

        connect(b.gainComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), [this,channel](int index) {
            this->scope->voltage[channel].gainStepIndex = (unsigned)index;
            emit gainChanged(channel, this->scope->gain(channel));
        });
        connect(b.invertCheckBox, &QAbstractButton::toggled, [this,channel](bool checked) {
            this->scope->voltage[channel].inverted = checked;
        });
        connect(b.miscComboBox, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), [this,channel,spec,scope](int index){
            if (channel < spec->channels) {
                this->scope->voltage[channel].couplingIndex = (unsigned)index;
                emit couplingChanged(channel, scope->coupling(channel, spec));
            } else {
                this->scope->voltage[channel].math = (Dso::MathMode) index;
                emit modeChanged(this->scope->voltage[channel].math);
            }
        });
        connect(b.usedCheckBox, &QAbstractButton::toggled, [this,channel](bool checked) {
            this->scope->voltage[channel].used = checked;
            emit usedChanged(channel, checked);
        });
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
    if (channel >= spec->channels) return;
    if (couplingIndex >= spec->couplings.size()) return;
    QSignalBlocker blocker(channelBlocks[channel].miscComboBox);
    channelBlocks[channel].miscComboBox->setCurrentIndex((int)couplingIndex);
}

void VoltageDock::setGain(ChannelID channel, unsigned gainStepIndex) {
    if (channel >= scope->voltage.size()) return;
    if (gainStepIndex >= scope->gainSteps.size()) return;
    QSignalBlocker blocker(channelBlocks[channel].gainComboBox);
    channelBlocks[channel].gainComboBox->setCurrentIndex((unsigned)gainStepIndex);
}

void VoltageDock::setMode(Dso::MathMode mode) {
    QSignalBlocker blocker(channelBlocks[spec->channels].miscComboBox);
    channelBlocks[spec->channels].miscComboBox->setCurrentIndex((int)mode);
}

void VoltageDock::setUsed(ChannelID channel, bool used) {
    if (channel >= scope->voltage.size()) return;
    QSignalBlocker blocker(channelBlocks[channel].usedCheckBox);
    channelBlocks[channel].usedCheckBox->setChecked(used);
}
