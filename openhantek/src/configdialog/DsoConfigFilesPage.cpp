// SPDX-License-Identifier: GPL-2.0+

#include "DsoConfigFilesPage.h"

DsoConfigFilesPage::DsoConfigFilesPage(DsoSettings *settings, QWidget *parent) : QWidget(parent), settings(settings) {
    // Export group
    screenColorCheckBox = new QCheckBox(tr("Export Images with Screen Colors"));
    screenColorCheckBox->setChecked(settings->view.screenColorImages);

    imageWidthLabel = new QLabel(tr("Image width"));
    imageWidthSpinBox = new QSpinBox();
    imageWidthSpinBox->setMinimum(100);
    imageWidthSpinBox->setMaximum(9999);
    imageWidthSpinBox->setValue(settings->exporting.imageSize.width());
    imageHeightLabel = new QLabel(tr("Image height"));
    imageHeightSpinBox = new QSpinBox();
    imageHeightSpinBox->setMinimum(100);
    imageHeightSpinBox->setMaximum(9999);
    imageHeightSpinBox->setValue(settings->exporting.imageSize.height());

    exportLayout = new QGridLayout();
    exportLayout->addWidget(screenColorCheckBox, 0, 0, 1, 2);
    exportLayout->addWidget(imageWidthLabel, 1, 0);
    exportLayout->addWidget(imageWidthSpinBox, 1, 1);
    exportLayout->addWidget(imageHeightLabel, 2, 0);
    exportLayout->addWidget(imageHeightSpinBox, 2, 1);

    exportGroup = new QGroupBox(tr("Export"));
    exportGroup->setLayout(exportLayout);

    // Configuration group
    saveOnExitCheckBox = new QCheckBox(tr("Save default settings on exit"));
    saveOnExitCheckBox->setChecked(settings->alwaysSave);
    saveNowButton = new QPushButton(tr("Save default settings now"));

    configurationLayout = new QVBoxLayout();
    configurationLayout->addWidget(saveOnExitCheckBox, 0);
    configurationLayout->addWidget(saveNowButton, 1);

    configurationGroup = new QGroupBox(tr("Configuration"));
    configurationGroup->setLayout(configurationLayout);

    // Main layout
    mainLayout = new QVBoxLayout();
    mainLayout->addWidget(exportGroup);
    mainLayout->addWidget(configurationGroup);
    mainLayout->addStretch(1);

    setLayout(mainLayout);

    connect(saveNowButton, &QAbstractButton::clicked, [settings]() { settings->save(); });
}

/// \brief Saves the new settings.
void DsoConfigFilesPage::saveSettings() {
    settings->alwaysSave = saveOnExitCheckBox->isChecked();
    settings->view.screenColorImages = screenColorCheckBox->isChecked();
    settings->exporting.imageSize.setWidth(imageWidthSpinBox->value());
    settings->exporting.imageSize.setHeight(imageHeightSpinBox->value());
}
