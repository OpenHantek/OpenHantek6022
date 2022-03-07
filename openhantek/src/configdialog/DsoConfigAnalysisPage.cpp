// SPDX-License-Identifier: GPL-2.0+

#include "DsoConfigAnalysisPage.h"

DsoConfigAnalysisPage::DsoConfigAnalysisPage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {
    // Initialize lists for comboboxes
    QStringList windowFunctionStrings;
    windowFunctionStrings << tr( "Rectangular" ) << tr( "Hann" ) << tr( "Hamming" ) << tr( "Cosine" ) << tr( "Lanczos" )
                          << tr( "Triangular" ) << tr( "Bartlett" ) << tr( "Bartlett-Hann" ) << tr( "Gauss" ) << tr( "Kaiser" )
                          << tr( "Blackman" ) << tr( "Nuttall" ) << tr( "Blackman-Harris" ) << tr( "Blackman-Nuttall" )
                          << tr( "Flat top" );

    // Initialize elements
    windowFunctionLabel = new QLabel( tr( "Window function" ) );
    windowFunctionComboBox = new QComboBox();
    windowFunctionComboBox->addItems( windowFunctionStrings );
    windowFunctionComboBox->setCurrentIndex( int( settings->post.spectrumWindow ) );

    referenceLevelLabel = new QLabel( tr( "Reference level<br/>"
                                          "&bull; 0 dBu = -2.2 dBV<br/>"
                                          "&bull; 0 dBm (@600 &Omega;) = -2.2 dBV<br/>"
                                          "&bull; 0 dBm (@50 &Omega;) = -13 dBV" ) );
    referenceLevelSpinBox = new QDoubleSpinBox();
    referenceLevelSpinBox->setDecimals( 1 );
    referenceLevelSpinBox->setMinimum( -100.0 );
    referenceLevelSpinBox->setMaximum( 100.0 );
    referenceLevelSpinBox->setValue( settings->post.spectrumReference );
    referenceLevelUnitLabel = new QLabel( tr( "dBV" ) );
    referenceLevelLayout = new QHBoxLayout();
    referenceLevelLayout->addWidget( referenceLevelSpinBox );
    referenceLevelLayout->addWidget( referenceLevelUnitLabel );

    minimumMagnitudeLabel = new QLabel( tr( "Minimum magnitude" ) );
    minimumMagnitudeSpinBox = new QDoubleSpinBox();
    minimumMagnitudeSpinBox->setDecimals( 1 );
    minimumMagnitudeSpinBox->setMinimum( -100.0 );
    minimumMagnitudeSpinBox->setMaximum( 100.0 );
    minimumMagnitudeSpinBox->setValue( settings->post.spectrumLimit );
    minimumMagnitudeUnitLabel = new QLabel( tr( "dBV" ) );
    minimumMagnitudeLayout = new QHBoxLayout();
    minimumMagnitudeLayout->addWidget( minimumMagnitudeSpinBox );
    minimumMagnitudeLayout->addWidget( minimumMagnitudeUnitLabel );

    reuseFftPlanCheckBox = new QCheckBox( tr( "Optimize FFT (slower startup, but lower CPU load)" ) );
    reuseFftPlanCheckBox->setChecked( settings->post.reuseFftPlan );

    spectrumLayout = new QGridLayout();
    spectrumLayout->addWidget( windowFunctionLabel, 0, 0 );
    spectrumLayout->addWidget( windowFunctionComboBox, 0, 1 );
    spectrumLayout->addWidget( referenceLevelLabel, 1, 0 );
    spectrumLayout->addLayout( referenceLevelLayout, 1, 1 );
    spectrumLayout->addWidget( minimumMagnitudeLabel, 2, 0 );
    spectrumLayout->addLayout( minimumMagnitudeLayout, 2, 1 );
    spectrumLayout->addWidget( reuseFftPlanCheckBox, 3, 0 );

    spectrumGroup = new QGroupBox( tr( "Spectrum" ) );
    spectrumGroup->setLayout( spectrumLayout );

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
    analysisLayout->addWidget( dummyLoadCheckbox, 0, 0 );
    analysisLayout->addLayout( dummyLoadLayout, 0, 1 );
    analysisLayout->addWidget( thdCheckBox, 1, 0 );
    analysisLayout->addWidget( showNoteCheckBox, 2, 0 );

    analysisGroup = new QGroupBox( tr( "Analysis" ) );
    analysisGroup->setLayout( analysisLayout );

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( spectrumGroup );
    mainLayout->addWidget( analysisGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
}


/// \brief Saves the new settings.
void DsoConfigAnalysisPage::saveSettings() {
    settings->post.spectrumWindow = Dso::WindowFunction( windowFunctionComboBox->currentIndex() );
    settings->post.spectrumReference = referenceLevelSpinBox->value();
    settings->post.spectrumLimit = minimumMagnitudeSpinBox->value();
    settings->scope.analysis.calculateDummyLoad = dummyLoadCheckbox->isChecked();
    settings->scope.analysis.dummyLoad = unsigned( dummyLoadSpinBox->value() );
    settings->scope.analysis.calculateTHD = thdCheckBox->isChecked();
    settings->post.reuseFftPlan = reuseFftPlanCheckBox->isChecked();
    settings->scope.analysis.showNoteValue = showNoteCheckBox->isChecked();
}
