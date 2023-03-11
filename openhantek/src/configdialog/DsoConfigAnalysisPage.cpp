// SPDX-License-Identifier: GPL-2.0-or-later

#include "DsoConfigAnalysisPage.h"

DsoConfigAnalysisPage::DsoConfigAnalysisPage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {

    // Spectrum group
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

    minimumMagnitudeLabel = new QLabel( tr( "Minimum magnitude" ) );
    minimumMagnitudeSpinBox = new QDoubleSpinBox();
    minimumMagnitudeSpinBox->setDecimals( 1 );
    minimumMagnitudeSpinBox->setMinimum( -100.0 );
    minimumMagnitudeSpinBox->setMaximum( 100.0 );
    minimumMagnitudeSpinBox->setValue( settings->analysis.spectrumLimit );
    minimumMagnitudeUnitLabel = new QLabel( tr( "dB" ) + settings->scope.analysis.dBsuffix() );
    minimumMagnitudeLayout = new QHBoxLayout();
    minimumMagnitudeLayout->addWidget( minimumMagnitudeSpinBox );
    minimumMagnitudeLayout->addWidget( minimumMagnitudeUnitLabel );

    reuseFftPlanCheckBox = new QCheckBox( tr( "Optimize FFT (slower startup, but lower CPU load)" ) );
    reuseFftPlanCheckBox->setChecked( settings->analysis.reuseFftPlan );

    spectrumLayout = new QGridLayout();
    int row = 0;
    spectrumLayout->addWidget( windowFunctionLabel, row, 0 );
    spectrumLayout->addWidget( windowFunctionComboBox, row, 1 );
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

    // Reference level group
    referenceLevelSpinBox = new QDoubleSpinBox();
    referenceLevelSpinBox->setDecimals( 1 );
    referenceLevelSpinBox->setMinimum( -100.0 );
    referenceLevelSpinBox->setMaximum( 100.0 );
    referenceLevelSpinBox->setValue( settings->scope.analysis.spectrumReference );
    referenceLevelUnitLabel = new QLabel( tr( "dBV" ) );
    referenceLevelLayout = new QHBoxLayout();
    referenceLevelLayout->addWidget( referenceLevelSpinBox );
    referenceLevelLayout->addWidget( referenceLevelUnitLabel );

    dBsuffixIndex = settings->scope.analysis.dBsuffixIndex;
    referenceLevelButtonLayout = new QGridLayout();
    dBVButton = new QPushButton( tr( "0 dBV" ) );
    dBVLabel = new QLabel( tr( "<p>= 1 Vrms</p>" ) );
    dBuButton = new QPushButton( tr( "0 dBu" ) );
    dBuLabel = new QLabel( tr( "<p>= -2.2 dBV (1 mW @ 600 &Omega;)</p>" ) );
    dBmButton = new QPushButton( tr( "0 dBm" ) );
    dBmLabel = new QLabel( tr( "<p>= -13 dBV (1 mW @ 50 &Omega;)</p>" ) );
    row = 0;
    referenceLevelButtonLayout->addWidget( dBVButton, row, 0 );
    referenceLevelButtonLayout->addWidget( dBVLabel, row, 1 );
    referenceLevelButtonLayout->addWidget( dBuButton, ++row, 0 );
    referenceLevelButtonLayout->addWidget( dBuLabel, row, 1 );
    referenceLevelButtonLayout->addWidget( dBmButton, ++row, 0 );
    referenceLevelButtonLayout->addWidget( dBmLabel, row, 1 );
    connect( dBVButton, &QPushButton::clicked, referenceLevelSpinBox, [ this ]() {
        referenceLevelSpinBox->setValue( 0.0 ); // set 0 dBV = 0 dBV
        dummyLoadSpinBox->setValue( 50 );       // set RF load 50 Ohm
        dBsuffixIndex = 0;
        minimumMagnitudeUnitLabel->setText( tr( "dB" ) + this->settings->scope.analysis.dBsuffix( dBsuffixIndex ) );
    } );
    connect( dBuButton, &QPushButton::clicked, referenceLevelSpinBox, [ this ]() {
        referenceLevelSpinBox->setValue( -2.2 ); // set 0 dBu = -2.2 dBV
        dummyLoadSpinBox->setValue( 600 );       // set telco load 600 Ohm
        dBsuffixIndex = 1;
        minimumMagnitudeUnitLabel->setText( tr( "dB" ) + this->settings->scope.analysis.dBsuffix( dBsuffixIndex ) );
    } );
    connect( dBmButton, &QPushButton::clicked, referenceLevelSpinBox, [ this ]() {
        referenceLevelSpinBox->setValue( -13.0 ); // set 0 dBm = -13 dBV
        dummyLoadSpinBox->setValue( 50 );         // set RF load 50 Ohm
        dBsuffixIndex = 2;
        minimumMagnitudeUnitLabel->setText( tr( "dB" ) + this->settings->scope.analysis.dBsuffix( dBsuffixIndex ) );
    } );

    referenceLayout = new QGridLayout();
    referenceLayout->addLayout( referenceLevelButtonLayout, 0, 0 );
    referenceLayout->addLayout( referenceLevelLayout, 0, 1 );
    referenceGroup = new QGroupBox( tr( "Set Reference Level" ) );
    referenceGroup->setLayout( referenceLayout );

    // Analysis group
    dummyLoadCheckbox = new QCheckBox( tr( "Calculate power dissipation for load resistance" ) );
    dummyLoadSpinBox = new QSpinBox();
    dummyLoadSpinBox->setMinimum( 1 );
    dummyLoadSpinBox->setMaximum( 1000 ); // range: audio (4/8 Ω), RF (50 Ω) and telco (600 Ω)
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

    // Put all in the main layout
    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( referenceGroup );
    mainLayout->addWidget( spectrumGroup );
    mainLayout->addWidget( analysisGroup );
    mainLayout->addWidget( cursorsGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
}


/// \brief Saves the new settings.
void DsoConfigAnalysisPage::saveSettings() {
    settings->scope.analysis.dBsuffixIndex = dBsuffixIndex;
    settings->scope.analysis.spectrumReference = referenceLevelSpinBox->value();
    settings->analysis.spectrumWindow = Dso::WindowFunction( windowFunctionComboBox->currentIndex() );
    settings->analysis.spectrumLimit = minimumMagnitudeSpinBox->value();
    settings->analysis.reuseFftPlan = reuseFftPlanCheckBox->isChecked();
    settings->scope.analysis.calculateDummyLoad = dummyLoadCheckbox->isChecked();
    settings->scope.analysis.dummyLoad = unsigned( dummyLoadSpinBox->value() );
    settings->scope.analysis.calculateTHD = thdCheckBox->isChecked();
    settings->scope.analysis.showNoteValue = showNoteCheckBox->isChecked();
    settings->view.cursorGridPosition = Qt::ToolBarArea( cursorsComboBox->currentData().toUInt() );
}
