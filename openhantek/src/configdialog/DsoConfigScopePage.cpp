// SPDX-License-Identifier: GPL-2.0-or-later

#include "DsoConfigScopePage.h"

DsoConfigScopePage::DsoConfigScopePage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {
    // Initialize lists for comboboxes
    QStringList interpolationStrings;
    interpolationStrings << tr( "Off" ) << tr( "Linear" ) << tr( "Step" ) << tr( "Sinc" );
    QList< double > timebaseSteps = { 1.0, 2.0, 5.0, 10.0 };

    // Horizontal group
    maxTimebaseLabel = new QLabel( tr( "Set slowest possible timebase" ) );
    maxTimebaseSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    maxTimebaseSiSpinBox->setSteps( timebaseSteps );
    maxTimebaseSiSpinBox->setMinimum( 0.1 );  // default 1000 ms/div (scopesettings.h)
    maxTimebaseSiSpinBox->setMaximum( 10.0 ); // possible steps: 100, 200, 500, 1000, 2000, 5000, 10000 ms
    maxTimebaseSiSpinBox->setValue( settings->scope.horizontal.maxTimebase );

    acquireIntervalLabel = new QLabel( tr( "Minimal time between captured frames<br/>(Longer times reduce the CPU load)" ) );
    acquireIntervalSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    acquireIntervalSiSpinBox->setSteps( timebaseSteps );
    acquireIntervalSiSpinBox->setMinimum( 1e-3 );   // minimal 1 ms holdOff
    acquireIntervalSiSpinBox->setMaximum( 100e-3 ); // up to 100 ms holdOff
    acquireIntervalSiSpinBox->setValue( settings->scope.horizontal.acquireInterval );

    // Graph group
    horizontalLayout = new QGridLayout();
    int row = 0;
    horizontalLayout->addWidget( maxTimebaseLabel, row, 0 );
    horizontalLayout->addWidget( maxTimebaseSiSpinBox, row, 1 );
    horizontalLayout->addWidget( acquireIntervalLabel, ++row, 0 );
    horizontalLayout->addWidget( acquireIntervalSiSpinBox, row, 1 );
    horizontalGroup = new QGroupBox( tr( "Horizontal" ) );
    horizontalGroup->setLayout( horizontalLayout );

    digitalPhosphorDepthLabel = new QLabel( tr( "Digital phosphor depth" ) );
    digitalPhosphorDepthSpinBox = new QSpinBox();
    digitalPhosphorDepthSpinBox->setMinimum( 2 );
    digitalPhosphorDepthSpinBox->setMaximum( 99 );
    digitalPhosphorDepthSpinBox->setValue( int( settings->view.digitalPhosphorDepth ) );
    interpolationLabel = new QLabel( tr( "Interpolation" ) );
    interpolationComboBox = new QComboBox();
    interpolationComboBox->addItems( interpolationStrings );
    interpolationComboBox->setCurrentIndex( settings->view.interpolation );

    graphLayout = new QGridLayout();
    row = 0;
    graphLayout->addWidget( digitalPhosphorDepthLabel, row, 0 );
    graphLayout->addWidget( digitalPhosphorDepthSpinBox, row, 1 );
    graphLayout->addWidget( interpolationLabel, ++row, 0 );
    graphLayout->addWidget( interpolationComboBox, row, 1 );

    graphGroup = new QGroupBox( tr( "Graph" ) );
    graphGroup->setLayout( graphLayout );

    // Zoom group
    zoomImageCheckBox = new QCheckBox( tr( "Export 1:1 zoomed screen in double height" ) );
    zoomImageCheckBox->setChecked( settings->view.zoomImage );
    zoomHeightLabel = new QLabel( tr( "Zoom area height factor" ) );
    zoomHeightComboBox = new QComboBox();
    zoomHeightComboBox->addItems( { "1", "2", "4", "8", "16" } );
    zoomHeightComboBox->setCurrentIndex( settings->view.zoomHeightIndex );
    exportScaleLabel = new QLabel( tr( "Upscale exported images" ) );
    exportScaleSpinBox = new QSpinBox();
    exportScaleSpinBox->setMinimum( 1 );
    exportScaleSpinBox->setMaximum( 16 );
    exportScaleSpinBox->setValue( settings->view.exportScaleValue );
    zoomLayout = new QGridLayout();
    row = 0;
    zoomLayout->addWidget( zoomHeightLabel, row, 0 );
    zoomLayout->addWidget( zoomHeightComboBox, row, 1 );
    ++row;
    zoomLayout->addWidget( zoomImageCheckBox, row, 0 );
    ++row;
    zoomLayout->addWidget( exportScaleLabel, row, 0 );
    zoomLayout->addWidget( exportScaleSpinBox, row, 1 );
    zoomGroup = new QGroupBox( tr( "Zoom" ) );
    zoomGroup->setLayout( zoomLayout );

    // Configuration group
    saveOnExitCheckBox = new QCheckBox( tr( "Save settings on exit" ) );
    saveOnExitCheckBox->setChecked( settings->alwaysSave );
    defaultSettingsCheckBox = new QCheckBox( tr( "Apply default settings after next restart" ) );
    defaultSettingsCheckBox->setChecked( 0 == settings->configVersion );
    saveNowButton = new QPushButton( tr( "Save settings now" ) );
    hasACmodificationCheckBox =
        new QCheckBox( tr( "Scope has hardware modification for AC coupling (restart needed to apply the change)" ) );
    hasACmodificationCheckBox->setChecked( settings->scope.hasACmodification );
    toolTipVisibleCheckBox = new QCheckBox( tr( "Show tooltips for user interface (restart needed to apply the change)" ) );
    toolTipVisibleCheckBox->setChecked( settings->scope.toolTipVisible );
    configurationLayout = new QGridLayout();
    row = 0;
    configurationLayout->addWidget( saveOnExitCheckBox, row, 0 );
    configurationLayout->addWidget( saveNowButton, row, 1 );
    configurationLayout->addWidget( defaultSettingsCheckBox, ++row, 0, 1, 2 );
    configurationLayout->addWidget( toolTipVisibleCheckBox, ++row, 0, 1, 2 );
    if ( settings->scope.hasACcoupling ) {
        hasACmodificationCheckBox->setChecked( true ); // check but do not show the box
    } else {
        configurationLayout->addWidget( hasACmodificationCheckBox, ++row, 0, 1, 2 ); // show it
    }
    configurationGroup = new QGroupBox( tr( "Configuration" ) );
    configurationGroup->setLayout( configurationLayout );

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( horizontalGroup );
    mainLayout->addWidget( graphGroup );
    mainLayout->addWidget( zoomGroup );
    mainLayout->addWidget( configurationGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
    connect( saveNowButton, &QAbstractButton::clicked, [ settings ]() { settings->save(); } );
}

/// \brief Saves the new settings.
void DsoConfigScopePage::saveSettings() {
    settings->scope.hasACmodification = hasACmodificationCheckBox->isChecked();
    settings->scope.toolTipVisible = toolTipVisibleCheckBox->isChecked();
    settings->scope.horizontal.maxTimebase = maxTimebaseSiSpinBox->value();
    settings->scope.horizontal.acquireInterval = acquireIntervalSiSpinBox->value();
    settings->view.interpolation = Dso::InterpolationMode( interpolationComboBox->currentIndex() );
    settings->view.digitalPhosphorDepth = unsigned( digitalPhosphorDepthSpinBox->value() );
    settings->alwaysSave = saveOnExitCheckBox->isChecked();
    if ( defaultSettingsCheckBox->isChecked() )
        settings->configVersion = 0;
    settings->view.zoomImage = zoomImageCheckBox->isChecked();
    settings->view.zoomHeightIndex = zoomHeightComboBox->currentIndex();
    settings->view.exportScaleValue = exportScaleSpinBox->value();
}
