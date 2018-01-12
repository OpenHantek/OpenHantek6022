// SPDX-License-Identifier: GPL-2.0+

#include "DsoConfigScopePage.h"

DsoConfigScopePage::DsoConfigScopePage(DsoSettings *settings, QWidget *parent) : QWidget(parent), settings(settings) {
    // Initialize lists for comboboxes
    QStringList interpolationStrings;
    interpolationStrings << tr("Off") << tr("Linear");

    // Initialize elements
    interpolationLabel = new QLabel(tr("Interpolation"));
    interpolationComboBox = new QComboBox();
    interpolationComboBox->addItems(interpolationStrings);
    interpolationComboBox->setCurrentIndex(settings->view.interpolation);
    digitalPhosphorDepthLabel = new QLabel(tr("Digital phosphor depth"));
    digitalPhosphorDepthSpinBox = new QSpinBox();
    digitalPhosphorDepthSpinBox->setMinimum(2);
    digitalPhosphorDepthSpinBox->setMaximum(99);
    digitalPhosphorDepthSpinBox->setValue(settings->view.digitalPhosphorDepth);

    graphLayout = new QGridLayout();
    graphLayout->addWidget(interpolationLabel, 1, 0);
    graphLayout->addWidget(interpolationComboBox, 1, 1);
    graphLayout->addWidget(digitalPhosphorDepthLabel, 2, 0);
    graphLayout->addWidget(digitalPhosphorDepthSpinBox, 2, 1);

    graphGroup = new QGroupBox(tr("Graph"));
    graphGroup->setLayout(graphLayout);

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget(graphGroup);
    mainLayout->addStretch(1);

    setLayout(mainLayout);
}

/// \brief Saves the new settings.
void DsoConfigScopePage::saveSettings() {
    settings->view.interpolation = (Dso::InterpolationMode)interpolationComboBox->currentIndex();
    settings->view.digitalPhosphorDepth = digitalPhosphorDepthSpinBox->value();
}
