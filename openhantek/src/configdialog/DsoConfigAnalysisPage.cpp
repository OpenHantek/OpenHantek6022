// SPDX-License-Identifier: GPL-2.0-or-later

#include "DsoConfigAnalysisPage.h"

DsoConfigAnalysisPage::DsoConfigAnalysisPage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {

    // Initialize lists for comboboxes
    QStringList windowFunctionStrings;
    for ( auto wf : Dso::WindowFunctionEnum ) {
        windowFunctionStrings.append( Dso::windowFunctionString( wf ) );
    }

    // Initialize elements
    windowFunctionLabel = new QLabel( tr( "Window function" ) );
    windowFunctionComboBox = new QComboBox();
    windowFunctionComboBox->addItems( windowFunctionStrings );
    windowFunctionComboBox->setCurrentIndex( int( settings->analysis.spectrumWindow ) );

    referenceLevelLabel = new QLabel( tr( "Reference level<br/>"
                                          "&bull; 0 dBu = -2.2 dBV<br/>"
                                          "&bull; 0 dBm (@600 &Omega;) = -2.2 dBV<br/>"
                                          "&bull; 0 dBm (@50 &Omega;) = -13 dBV" ) );
    referenceLevelSpinBox = new QDoubleSpinBox();
    referenceLevelSpinBox->setDecimals( 1 );
    referenceLevelSpinBox->setMinimum( -100.0 );
    referenceLevelSpinBox->setMaximum( 100.0 );
    referenceLevelSpinBox->setValue( settings->analysis.spectrumReference );
    referenceLevelUnitLabel = new QLabel( tr( "dBV" ) );
    referenceLevelLayout = new QHBoxLayout();
    referenceLevelLayout->addWidget( referenceLevelSpinBox );
    referenceLevelLayout->addWidget( referenceLevelUnitLabel );

    minimumMagnitudeLabel = new QLabel( tr( "Minimum magnitude" ) );
    minimumMagnitudeSpinBox = new QDoubleSpinBox();
    minimumMagnitudeSpinBox->setDecimals( 1 );
    minimumMagnitudeSpinBox->setMinimum( -100.0 );
    minimumMagnitudeSpinBox->setMaximum( 100.0 );
    minimumMagnitudeSpinBox->setValue( settings->analysis.spectrumLimit );
    minimumMagnitudeUnitLabel = new QLabel( tr( "dBV" ) );
    minimumMagnitudeLayout = new QHBoxLayout();
    minimumMagnitudeLayout->addWidget( minimumMagnitudeSpinBox );
    minimumMagnitudeLayout->addWidget( minimumMagnitudeUnitLabel );

    reuseFftPlanCheckBox = new QCheckBox( tr( "Optimize FFT (slower startup, but lower CPU load)" ) );
    reuseFftPlanCheckBox->setChecked( settings->analysis.reuseFftPlan );

    spectrumLayout = new QGridLayout();
    int row = 0;
    spectrumLayout->addWidget( windowFunctionLabel, row, 0 );
    spectrumLayout->addWidget( windowFunctionComboBox, row, 1 );
    spectrumLayout->addWidget( referenceLevelLabel, ++row, 0 );
    spectrumLayout->addLayout( referenceLevelLayout, row, 1 );
    spectrumLayout->addWidget( minimumMagnitudeLabel, ++row, 0 );
    spectrumLayout->addLayout( minimumMagnitudeLayout, row, 1 );
    spectrumLayout->addWidget( reuseFftPlanCheckBox, ++row, 0 );

    spectrumGroup = new QGroupBox( tr( "Spectrum" ) );
    spectrumGroup->setLayout( spectrumLayout );

    // Cursor measurement group
    cursorsLabel = new QLabel( tr( "Position" ) );
    cursorsComboBox = new QComboBox();
    cursorsComboBox->addItem( tr( "Left" ), Qt::LeftToolBarArea );
    cursorsComboBox->addItem( tr( "Right" ), Qt::RightToolBarArea );
    cursorsComboBox->setCurrentIndex( settings->view.cursorGridPosition == Qt::LeftToolBarArea ? 0 : 1 );
    cursorsLayout = new QGridLayout();
    row = 0;
    cursorsLayout->addWidget( cursorsLabel, row, 0 );
    cursorsLayout->addWidget( cursorsComboBox, row, 1 );
    cursorsGroup = new QGroupBox( tr( "Cursors" ) );
    cursorsGroup->setLayout( cursorsLayout );

    dummyLoadCheckbox = new QCheckBox( tr( "Calculate power dissipation for load resistance" ) );
    dummyLoadSpinBox = new QSpinBox();
    dummyLoadSpinBox->setMinimum( 1 );
    dummyLoadSpinBox->setMaximum( 1000 );                    // range: audio (4/8 Ω), RF (50 Ω) and telco (600 Ω)
    if ( 0 == settings->scope.analysis.dummyLoad ) {         // 0 was off in earlier setups
        settings->scope.analysis.calculateDummyLoad = false; // do not analyse
        settings->scope.analysis.dummyLoad = 50;             // set default
    }
    dummyLoadCheckbox->setChecked( settings->scope.analysis.calculateDummyLoad );
    dummyLoadSpinBox->setValue( int( settings->scope.analysis.dummyLoad ) );
    dummyLoadUnitLabel = new QLabel( tr( "<p>&Omega;</p>" ) );
    dummyLoadLayout = new QHBoxLayout();
    dummyLoadLayout->addWidget( dummyLoadSpinBox );
    dummyLoadLayout->addWidget( dummyLoadUnitLabel );

    thdCheckBox = new QCheckBox( tr( "Calculate total harmonic distortion (THD)" ) );
    thdCheckBox->setChecked( settings->scope.analysis.calculateTHD );

    showNoteCheckBox = new QCheckBox( tr( "Show note values for audio frequencies" ) );
    showNoteCheckBox->setChecked( settings->scope.analysis.showNoteValue );

    analysisLayout = new QGridLayout();
    row = 0;
    analysisLayout->addWidget( dummyLoadCheckbox, row, 0 );
    analysisLayout->addLayout( dummyLoadLayout, row, 1 );
    analysisLayout->addWidget( thdCheckBox, ++row, 0 );
    analysisLayout->addWidget( showNoteCheckBox, ++row, 0 );

    analysisGroup = new QGroupBox( tr( "Analysis" ) );
    analysisGroup->setLayout( analysisLayout );

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( spectrumGroup );
    mainLayout->addWidget( cursorsGroup );
    mainLayout->addWidget( analysisGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
}


/// \brief Saves the new settings.
void DsoConfigAnalysisPage::saveSettings() {
    settings->analysis.spectrumWindow = Dso::WindowFunction( windowFunctionComboBox->currentIndex() );
    settings->analysis.spectrumReference = referenceLevelSpinBox->value();
    settings->analysis.spectrumLimit = minimumMagnitudeSpinBox->value();
    settings->scope.analysis.calculateDummyLoad = dummyLoadCheckbox->isChecked();
    settings->scope.analysis.dummyLoad = unsigned( dummyLoadSpinBox->value() );
    settings->scope.analysis.calculateTHD = thdCheckBox->isChecked();
    settings->analysis.reuseFftPlan = reuseFftPlanCheckBox->isChecked();
    settings->scope.analysis.showNoteValue = showNoteCheckBox->isChecked();
    settings->view.cursorGridPosition = Qt::ToolBarArea( cursorsComboBox->currentData().toUInt() );
}
