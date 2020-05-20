// SPDX-License-Identifier: GPL-2.0+

#include "DsoConfigFilePage.h"

DsoConfigFilePage::DsoConfigFilePage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {
    // Export group
    zoomImageCheckBox = new QCheckBox( tr( "Export zoomed screen in double height" ) );
    zoomImageCheckBox->setChecked( settings->view.zoomImage );

    imageWidthLabel = new QLabel( tr( "Image width" ) );
    imageWidthSpinBox = new QSpinBox();
    imageWidthSpinBox->setMinimum( 100 );
    imageWidthSpinBox->setMaximum( 9999 );
    imageWidthSpinBox->setValue( settings->exporting.imageSize.width() );
    imageHeightLabel = new QLabel( tr( "Image height" ) );
    imageHeightSpinBox = new QSpinBox();
    imageHeightSpinBox->setMinimum( 100 );
    imageHeightSpinBox->setMaximum( 9999 );
    imageHeightSpinBox->setValue( settings->exporting.imageSize.height() );

    exportLayout = new QGridLayout();
    exportLayout->addWidget( imageWidthLabel, 0, 0 );
    exportLayout->addWidget( imageWidthSpinBox, 0, 1 );
    exportLayout->addWidget( imageHeightLabel, 1, 0 );
    exportLayout->addWidget( imageHeightSpinBox, 1, 1 );
    exportLayout->addWidget( zoomImageCheckBox, 2, 0, 1, 2 );

    exportGroup = new QGroupBox( tr( "Export" ) );
    exportGroup->setLayout( exportLayout );

    // Configuration group
    saveOnExitCheckBox = new QCheckBox( tr( "Save default settings on exit" ) );
    saveOnExitCheckBox->setChecked( settings->alwaysSave );
    saveNowButton = new QPushButton( tr( "Save default settings now" ) );

    configurationLayout = new QVBoxLayout();
    configurationLayout->addWidget( saveOnExitCheckBox, 0 );
    configurationLayout->addWidget( saveNowButton, 1 );

    configurationGroup = new QGroupBox( tr( "Configuration" ) );
    configurationGroup->setLayout( configurationLayout );

    // Main layout
    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( exportGroup );
    mainLayout->addWidget( configurationGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );

    connect( saveNowButton, &QAbstractButton::clicked, [settings]() { settings->save(); } );
}

/// \brief Saves the new settings.
void DsoConfigFilePage::saveSettings() {
    settings->alwaysSave = saveOnExitCheckBox->isChecked();
    settings->view.zoomImage = zoomImageCheckBox->isChecked();
    settings->exporting.imageSize.setWidth( imageWidthSpinBox->value() );
    settings->exporting.imageSize.setHeight( imageHeightSpinBox->value() );
}
