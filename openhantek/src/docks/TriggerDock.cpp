// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "TriggerDock.h"
#include "dockwindows.h"

#include "settings.h"
#include "sispinbox.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"

////////////////////////////////////////////////////////////////////////////////
// class TriggerDock
/// \brief Initializes the trigger settings docking window.
/// \param settings The target settings object.
/// \param specialTriggers The names of the special trigger sources.
/// \param parent The parent widget.
/// \param flags Flags for the window manager.
TriggerDock::TriggerDock(DsoSettings *settings, const std::vector<std::string> &specialTriggers, QWidget *parent,
                         Qt::WindowFlags flags)
    : QDockWidget(tr("Trigger"), parent, flags), settings(settings) {

    // Initialize lists for comboboxes
    for (unsigned int channel = 0; channel < settings->scope.physicalChannels; ++channel)
        this->sourceStandardStrings << tr("CH%1").arg(channel + 1);
    for(const std::string& name: specialTriggers)
        this->sourceSpecialStrings.append(QString::fromStdString(name));

    // Initialize elements
    this->modeLabel = new QLabel(tr("Mode"));
    this->modeComboBox = new QComboBox();
    for (int mode = Dso::TRIGGERMODE_AUTO; mode < Dso::TRIGGERMODE_COUNT; ++mode)
        this->modeComboBox->addItem(Dso::triggerModeString((Dso::TriggerMode)mode));

    this->slopeLabel = new QLabel(tr("Slope"));
    this->slopeComboBox = new QComboBox();
    for (int slope = Dso::SLOPE_POSITIVE; slope < Dso::SLOPE_COUNT; ++slope)
        this->slopeComboBox->addItem(Dso::slopeString((Dso::Slope)slope));

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

    // Connect signals and slots
    connect(this->modeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(modeSelected(int)));
    connect(this->slopeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slopeSelected(int)));
    connect(this->sourceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceSelected(int)));

    // Set values
    this->setMode(settings->scope.trigger.mode);
    this->setSlope(settings->scope.trigger.slope);
    this->setSource(settings->scope.trigger.special, settings->scope.trigger.source);
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void TriggerDock::closeEvent(QCloseEvent *event) {
    this->hide();

    event->accept();
}

/// \brief Changes the trigger mode if the new mode is supported.
/// \param mode The trigger mode.
/// \return Index of mode-value, -1 on error.
int TriggerDock::setMode(Dso::TriggerMode mode) {
    if (mode >= Dso::TRIGGERMODE_AUTO && mode < Dso::TRIGGERMODE_COUNT) {
        this->modeComboBox->setCurrentIndex(mode);
        return mode;
    }

    return -1;
}

/// \brief Changes the trigger slope if the new slope is supported.
/// \param slope The trigger slope.
/// \return Index of slope-value, -1 on error.
int TriggerDock::setSlope(Dso::Slope slope) {
    if (slope >= Dso::SLOPE_POSITIVE && slope <= Dso::SLOPE_NEGATIVE) {
        this->slopeComboBox->setCurrentIndex(slope);
        return slope;
    }

    return -1;
}

/// \brief Changes the trigger source if the new source is supported.
/// \param special true for a special channel (EXT, ...) as trigger source.
/// \param id The number of the channel, that should be used as trigger.
/// \return Index of source item, -1 on error.
int TriggerDock::setSource(bool special, unsigned int id) {
    if ((!special && id >= (unsigned int)this->sourceStandardStrings.count()) ||
        (special && id >= (unsigned int)this->sourceSpecialStrings.count()))
        return -1;

    int index = id;
    if (special) index += this->sourceStandardStrings.count();
    this->sourceComboBox->setCurrentIndex(index);

    return index;
}

/// \brief Called when the mode combo box changes it's value.
/// \param index The index of the combo box item.
void TriggerDock::modeSelected(int index) {
    settings->scope.trigger.mode = (Dso::TriggerMode)index;
    emit modeChanged(settings->scope.trigger.mode);
}

/// \brief Called when the slope combo box changes it's value.
/// \param index The index of the combo box item.
void TriggerDock::slopeSelected(int index) {
    settings->scope.trigger.slope = (Dso::Slope)index;
    emit slopeChanged(settings->scope.trigger.slope);
}

/// \brief Called when the source combo box changes it's value.
/// \param index The index of the combo box item.
void TriggerDock::sourceSelected(int index) {
    unsigned int id = index;
    bool special = false;

    if (id >= (unsigned int)this->sourceStandardStrings.count()) {
        id -= this->sourceStandardStrings.count();
        special = true;
    }

    settings->scope.trigger.source = id;
    settings->scope.trigger.special = special;
    emit sourceChanged(special, id);
}
