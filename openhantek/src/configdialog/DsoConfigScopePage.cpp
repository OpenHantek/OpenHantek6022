// SPDX-License-Identifier: GPL-2.0+

#include "DsoConfigScopePage.h"

DsoConfigScopePage::DsoConfigScopePage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {
    // Initialize lists for comboboxes
    QStringList interpolationStrings;
    interpolationStrings << tr( "Off" ) << tr( "Linear" );
    QList< double > slowTimebaseSteps = {1.0, 2.0, 5.0, 10.0};

    maxTimebaseLabel = new QLabel( tr( "Set slowest possible timebase\n(GUI may become very unresponsible!)" ) );
    maxTimebaseSiSpinBox = new SiSpinBox();
    maxTimebaseSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    maxTimebaseSiSpinBox->setSteps( slowTimebaseSteps );
    maxTimebaseSiSpinBox->setMinimum( 0.1 ); // default 100 ms/div
    maxTimebaseSiSpinBox->setMaximum( 1.0 );

    maxTimebaseSiSpinBox->setValue( settings->scope.horizontal.maxTimebase );
    timebaseLayout = new QGridLayout();
    timebaseLayout->addWidget( maxTimebaseLabel, 0, 0 );
    timebaseLayout->addWidget( maxTimebaseSiSpinBox, 0, 1 );
    timebaseGroup = new QGroupBox( tr( "Horizontal" ) );
    timebaseGroup->setLayout( timebaseLayout );

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

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( timebaseGroup );
    mainLayout->addWidget( graphGroup );
    mainLayout->addWidget( cursorsGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
}

/// \brief Saves the new settings.
void DsoConfigScopePage::saveSettings() {
    settings->scope.horizontal.maxTimebase = maxTimebaseSiSpinBox->value();
    settings->view.interpolation = Dso::InterpolationMode( interpolationComboBox->currentIndex() );
    settings->view.digitalPhosphorDepth = unsigned( digitalPhosphorDepthSpinBox->value() );
    settings->view.cursorGridPosition = Qt::ToolBarArea( cursorsComboBox->currentData().toUInt() );
}
