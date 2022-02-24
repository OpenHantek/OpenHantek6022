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

    dummyLoadLabel = new QLabel( tr( "Calculate power dissipation for load resistance<br/>(0 = off)" ) );
    dummyLoadSpinBox = new QSpinBox();
    dummyLoadSpinBox->setMinimum( 0 );    // 0 = off
    dummyLoadSpinBox->setMaximum( 1000 ); // range: audio (4/8 Ω), RF (50 Ω) and telco (600 Ω)
    dummyLoadSpinBox->setValue( int( settings->scope.analysis.dummyLoad ) );
    dummyLoadUnitLabel = new QLabel( tr( "<p>&Omega;</p>" ) );
    dummyLoadLayout = new QHBoxLayout();
    dummyLoadLayout->addWidget( dummyLoadSpinBox );
    dummyLoadLayout->addWidget( dummyLoadUnitLabel );

    thdCheckBox = new QCheckBox( tr( "Calculate total harmonic distortion (THD)" ) );
    thdCheckBox->setChecked( settings->scope.analysis.calculateTHD );

    powerLayout = new QGridLayout();
    powerLayout->addWidget( dummyLoadLabel, 0, 0 );
    powerLayout->addLayout( dummyLoadLayout, 0, 1 );
    powerLayout->addWidget( thdCheckBox, 1, 0 );

    powerGroup = new QGroupBox( tr( "Power" ) );
    powerGroup->setLayout( powerLayout );

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( spectrumGroup );
    mainLayout->addWidget( powerGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
}


/// \brief Saves the new settings.
void DsoConfigAnalysisPage::saveSettings() {
    settings->post.spectrumWindow = Dso::WindowFunction( windowFunctionComboBox->currentIndex() );
    settings->post.spectrumReference = referenceLevelSpinBox->value();
    settings->post.spectrumLimit = minimumMagnitudeSpinBox->value();
    settings->scope.analysis.dummyLoad = unsigned( dummyLoadSpinBox->value() );
    settings->scope.analysis.calculateTHD = thdCheckBox->isChecked();
    settings->post.reuseFftPlan = reuseFftPlanCheckBox->isChecked();
}
