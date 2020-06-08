// SPDX-License-Identifier: GPL-2.0+

#include "DsoConfigScopePage.h"

DsoConfigScopePage::DsoConfigScopePage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {
    // Initialize lists for comboboxes
    QStringList interpolationStrings;
    interpolationStrings << tr( "Off" ) << tr( "Linear" );
    QList< double > timebaseSteps = {1.0, 2.0, 5.0, 10.0};

    maxTimebaseLabel = new QLabel( tr( "Set slowest possible timebase" ) );
    maxTimebaseSiSpinBox = new SiSpinBox();
    maxTimebaseSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    maxTimebaseSiSpinBox->setSteps( timebaseSteps );
    maxTimebaseSiSpinBox->setMinimum( 0.1 ); // default 100 ms/div
    maxTimebaseSiSpinBox->setMaximum( 5.0 ); // possible steps: 200, 500, 1000, 2000, 5000 ms
    maxTimebaseSiSpinBox->setValue( settings->scope.horizontal.maxTimebase );

    acquireIntervalLabel = new QLabel( tr( "Minimal time between captured frames<br/>(Longer times reduce the CPU load)" ) );
    acquireIntervalSiSpinBox = new SiSpinBox();
    acquireIntervalSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    acquireIntervalSiSpinBox->setSteps( timebaseSteps );
    acquireIntervalSiSpinBox->setMinimum( 500e-6 ); // double 500 µs -> unsigned 0 ms delay
    acquireIntervalSiSpinBox->setMaximum( 50e-3 );  // 50 ms delay
    acquireIntervalSiSpinBox->setValue( settings->scope.horizontal.acquireInterval );

    horizontalLayout = new QGridLayout();
    horizontalLayout->addWidget( maxTimebaseLabel, 0, 0 );
    horizontalLayout->addWidget( maxTimebaseSiSpinBox, 0, 1 );
    horizontalLayout->addWidget( acquireIntervalLabel, 1, 0 );
    horizontalLayout->addWidget( acquireIntervalSiSpinBox, 1, 1 );
    horizontalGroup = new QGroupBox( tr( "Horizontal" ) );
    horizontalGroup->setLayout( horizontalLayout );

    interpolationLabel = new QLabel( tr( "Interpolation" ) );
    interpolationComboBox = new QComboBox();
    interpolationComboBox->addItems( interpolationStrings );
    interpolationComboBox->setCurrentIndex( settings->view.interpolation );
    digitalPhosphorDepthLabel = new QLabel( tr( "Digital phosphor depth" ) );
    digitalPhosphorDepthSpinBox = new QSpinBox();
    digitalPhosphorDepthSpinBox->setMinimum( 2 );
    digitalPhosphorDepthSpinBox->setMaximum( 99 );
    digitalPhosphorDepthSpinBox->setValue( int( settings->view.digitalPhosphorDepth ) );

    graphLayout = new QGridLayout();
    graphLayout->addWidget( interpolationLabel, 1, 0 );
    graphLayout->addWidget( interpolationComboBox, 1, 1 );
    graphLayout->addWidget( digitalPhosphorDepthLabel, 2, 0 );
    graphLayout->addWidget( digitalPhosphorDepthSpinBox, 2, 1 );

    graphGroup = new QGroupBox( tr( "Graph" ) );
    graphGroup->setLayout( graphLayout );

    cursorsLabel = new QLabel( tr( "Position" ) );
    cursorsComboBox = new QComboBox();
    cursorsComboBox->addItem( tr( "Left" ), Qt::LeftToolBarArea );
    cursorsComboBox->addItem( tr( "Right" ), Qt::RightToolBarArea );
    cursorsComboBox->setCurrentIndex( settings->view.cursorGridPosition == Qt::LeftToolBarArea ? 0 : 1 );

    cursorsLayout = new QGridLayout();
    cursorsLayout->addWidget( cursorsLabel, 0, 0 );
    cursorsLayout->addWidget( cursorsComboBox, 0, 1 );

    cursorsGroup = new QGroupBox( tr( "Cursors" ) );
    cursorsGroup->setLayout( cursorsLayout );

    // Export group
    zoomImageCheckBox = new QCheckBox( tr( "Export zoomed screen in double height" ) );
    zoomImageCheckBox->setChecked( settings->view.zoomImage );
    exportLayout = new QGridLayout();
    exportLayout->addWidget( zoomImageCheckBox, 2, 0, 1, 2 );

    exportGroup = new QGroupBox( tr( "Export" ) );
    exportGroup->setLayout( exportLayout );

    // Configuration group
    saveOnExitCheckBox = new QCheckBox( tr( "Save settings on exit" ) );
    saveOnExitCheckBox->setChecked( settings->alwaysSave );
    defaultSettingsCheckBox = new QCheckBox( tr( "Apply default settings after next restart" ) );
    defaultSettingsCheckBox->setChecked( 0 == settings->configVersion );
    saveNowButton = new QPushButton( tr( "Save settings now" ) );

    configurationLayout = new QVBoxLayout();
    configurationLayout->addWidget( saveOnExitCheckBox, 0 );
    configurationLayout->addWidget( defaultSettingsCheckBox, 1 );
    configurationLayout->addWidget( saveNowButton, 2 );

    configurationGroup = new QGroupBox( tr( "Configuration" ) );
    configurationGroup->setLayout( configurationLayout );


    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( configurationGroup );
    mainLayout->addWidget( horizontalGroup );
    mainLayout->addWidget( graphGroup );
    mainLayout->addWidget( exportGroup );
    mainLayout->addWidget( cursorsGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
    connect( saveNowButton, &QAbstractButton::clicked, [settings]() { settings->save(); } );
}

/// \brief Saves the new settings.
void DsoConfigScopePage::saveSettings() {
    settings->scope.horizontal.maxTimebase = maxTimebaseSiSpinBox->value();
    settings->scope.horizontal.acquireInterval = acquireIntervalSiSpinBox->value();
    settings->view.interpolation = Dso::InterpolationMode( interpolationComboBox->currentIndex() );
    settings->view.digitalPhosphorDepth = unsigned( digitalPhosphorDepthSpinBox->value() );
    settings->view.cursorGridPosition = Qt::ToolBarArea( cursorsComboBox->currentData().toUInt() );
    settings->alwaysSave = saveOnExitCheckBox->isChecked();
    if ( defaultSettingsCheckBox->isChecked() )
        settings->configVersion = 0;
    settings->view.zoomImage = zoomImageCheckBox->isChecked();
}
