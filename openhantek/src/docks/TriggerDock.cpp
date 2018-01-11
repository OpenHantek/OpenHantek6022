// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "TriggerDock.h"
#include "dockwindows.h"

#include "hantekdso/controlspecification.h"
#include "settings.h"
#include "sispinbox.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"

TriggerDock::TriggerDock(DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent,
                         Qt::WindowFlags flags)
    : QDockWidget(tr("Trigger"), parent, flags), scope(scope), mSpec(spec) {

    // Initialize lists for comboboxes
    for (ChannelID channel = 0; channel < mSpec->channels; ++channel)
        this->sourceStandardStrings << tr("CH%1").arg(channel + 1);
    for (const Dso::SpecialTriggerChannel &specialTrigger : mSpec->specialTriggerChannels)
        this->sourceSpecialStrings.append(QString::fromStdString(specialTrigger.name));

    // Initialize elements
    this->modeLabel = new QLabel(tr("Mode"));
    this->modeComboBox = new QComboBox();
    for (Dso::TriggerMode mode : mSpec->triggerModes) this->modeComboBox->addItem(Dso::triggerModeString(mode));

    this->slopeLabel = new QLabel(tr("Slope"));
    this->slopeComboBox = new QComboBox();
    for (Dso::Slope slope : Dso::SlopeEnum) this->slopeComboBox->addItem(Dso::slopeString(slope));

    this->sourceLabel = new QLabel(tr("Source"));
    this->sourceComboBox = new QComboBox();
    this->sourceComboBox->addItems(this->sourceStandardStrings);
    this->sourceComboBox->addItems(this->sourceSpecialStrings);

    this->dockLayout = new QGridLayout();
    this->dockLayout->setColumnMinimumWidth(0, 64);
    this->dockLayout->setColumnStretch(1, 1);
    this->dockLayout->addWidget(this->modeLabel, 0, 0);
    this->dockLayout->addWidget(this->modeComboBox, 0, 1);
    this->dockLayout->addWidget(this->sourceLabel, 1, 0);
    this->dockLayout->addWidget(this->sourceComboBox, 1, 1);
    this->dockLayout->addWidget(this->slopeLabel, 2, 0);
    this->dockLayout->addWidget(this->slopeComboBox, 2, 1);

    this->dockWidget = new QWidget();
    SetupDockWidget(this, dockWidget, dockLayout);

    // Set values
    setMode(scope->trigger.mode);
    setSlope(scope->trigger.slope);
    setSource(scope->trigger.special, scope->trigger.source);

    // Connect signals and slots
    connect(this->modeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this, spec](int index) {
                this->scope->trigger.mode = mSpec->triggerModes[(unsigned)index];
                emit modeChanged(this->scope->trigger.mode);
            });
    connect(this->slopeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this](int index) {
                this->scope->trigger.slope = (Dso::Slope)index;
                emit slopeChanged(this->scope->trigger.slope);
            });
    connect(this->sourceComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this](int index) {
                bool special = false;

                if (index >= this->sourceStandardStrings.count()) {
                    index -= this->sourceStandardStrings.count();
                    special = true;
                }

                this->scope->trigger.source = (unsigned)index;
                this->scope->trigger.special = special;
                emit sourceChanged(special, (unsigned)index);
            });
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void TriggerDock::closeEvent(QCloseEvent *event) {
    this->hide();

    event->accept();
}

void TriggerDock::setMode(Dso::TriggerMode mode) {
    int index = std::find(mSpec->triggerModes.begin(), mSpec->triggerModes.end(), mode) - mSpec->triggerModes.begin();
    QSignalBlocker blocker(modeComboBox);
    modeComboBox->setCurrentIndex(index);
}

void TriggerDock::setSlope(Dso::Slope slope) {
    QSignalBlocker blocker(slopeComboBox);
    slopeComboBox->setCurrentIndex((int)slope);
}

void TriggerDock::setSource(bool special, unsigned int id) {
    if ((!special && id >= (unsigned int)this->sourceStandardStrings.count()) ||
        (special && id >= (unsigned int)this->sourceSpecialStrings.count()))
        return;

    int index = (int)id;
    if (special) index += this->sourceStandardStrings.count();
    QSignalBlocker blocker(sourceComboBox);
    sourceComboBox->setCurrentIndex(index);
}
