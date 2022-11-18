// SPDX-License-Identifier: GPL-2.0-or-later

#include "DsoConfigColorsPage.h"

DsoConfigColorsPage::DsoConfigColorsPage( DsoSettings *settings, QWidget *parent ) : QWidget( parent ), settings( settings ) {
    // Initialize elements
    DsoSettingsView &colorSettings = settings->view;
    enum { COL_LABEL = 0, COL_SCR_CHANNEL, COL_SCR_SPECTRUM, COL_PRT_CHANNEL, COL_PRT_SPECTRUM };

    // Plot Area
    screenColorsLabel = new QLabel( tr( "Screen" ) );
    screenColorsLabel->setAlignment( Qt::AlignHCenter );
    printColorsLabel = new QLabel( tr( "Print" ) );
    printColorsLabel->setAlignment( Qt::AlignHCenter );

    backgroundLabel = new QLabel( tr( "Background" ) );
    backgroundColorBox = new ColorBox( colorSettings.screen.background );
    printBackgroundColorBox = new ColorBox( colorSettings.print.background );

    gridLabel = new QLabel( tr( "Grid" ) );
    gridColorBox = new ColorBox( colorSettings.screen.grid );
    printGridColorBox = new ColorBox( colorSettings.print.grid );

    axesLabel = new QLabel( tr( "Axes" ) );
    axesColorBox = new ColorBox( colorSettings.screen.axes );
    printAxesColorBox = new ColorBox( colorSettings.print.axes );

    borderLabel = new QLabel( tr( "Border" ) );
    borderColorBox = new ColorBox( colorSettings.screen.border );
    printBorderColorBox = new ColorBox( colorSettings.print.border );

    markersLabel = new QLabel( tr( "Markers" ) );
    markersColorBox = new ColorBox( colorSettings.screen.markers );
    printMarkersColorBox = new ColorBox( colorSettings.print.markers );

    textLabel = new QLabel( tr( "Text" ) );
    textColorBox = new ColorBox( colorSettings.screen.text );
    printTextColorBox = new ColorBox( colorSettings.print.text );

    // Graph category
    screenChannelLabel = new QLabel( tr( "Voltage" ) );
    screenChannelLabel->setAlignment( Qt::AlignHCenter );
    screenSpectrumLabel = new QLabel( tr( "Spectrum" ) );
    screenSpectrumLabel->setAlignment( Qt::AlignHCenter );
    printChannelLabel = new QLabel( tr( "Voltage" ) );
    printChannelLabel->setAlignment( Qt::AlignHCenter );
    printSpectrumLabel = new QLabel( tr( "Spectrum" ) );
    printSpectrumLabel->setAlignment( Qt::AlignHCenter );

    for ( ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel ) {
        colorLabel.push_back( new QLabel( settings->scope.voltage[ channel ].name ) );
        screenChannelColorBox.push_back( new ColorBox( colorSettings.screen.voltage[ channel ] ) );
        screenSpectrumColorBox.push_back( new ColorBox( colorSettings.screen.spectrum[ channel ] ) );
        printChannelColorBox.push_back( new ColorBox( colorSettings.print.voltage[ channel ] ) );
        printSpectrumColorBox.push_back( new ColorBox( colorSettings.print.spectrum[ channel ] ) );
    }

    // Plot Area Layout
    colorsLayout = new QGridLayout();
    colorsLayout->setColumnStretch( COL_LABEL, 1 );
    colorsLayout->setColumnMinimumWidth( COL_SCR_CHANNEL, 80 );
    colorsLayout->setColumnMinimumWidth( COL_SCR_SPECTRUM, 80 );
    colorsLayout->setColumnMinimumWidth( COL_PRT_CHANNEL, 80 );
    colorsLayout->setColumnMinimumWidth( COL_PRT_SPECTRUM, 80 );

    int row = 0;
    colorsLayout->addWidget( screenColorsLabel, row, COL_SCR_CHANNEL, 1, 2 );
    colorsLayout->addWidget( printColorsLabel, row, COL_PRT_CHANNEL, 1, 2 );
    ++row;
    colorsLayout->addWidget( backgroundLabel, row, COL_LABEL );
    colorsLayout->addWidget( backgroundColorBox, row, COL_SCR_CHANNEL, 1, 2 );
    colorsLayout->addWidget( printBackgroundColorBox, row, COL_PRT_CHANNEL, 1, 2 );
    ++row;
    colorsLayout->addWidget( gridLabel, row, COL_LABEL );
    colorsLayout->addWidget( gridColorBox, row, COL_SCR_CHANNEL, 1, 2 );
    colorsLayout->addWidget( printGridColorBox, row, COL_PRT_CHANNEL, 1, 2 );
    ++row;
    colorsLayout->addWidget( axesLabel, row, COL_LABEL );
    colorsLayout->addWidget( axesColorBox, row, COL_SCR_CHANNEL, 1, 2 );
    colorsLayout->addWidget( printAxesColorBox, row, COL_PRT_CHANNEL, 1, 2 );
    ++row;
    colorsLayout->addWidget( borderLabel, row, COL_LABEL );
    colorsLayout->addWidget( borderColorBox, row, COL_SCR_CHANNEL, 1, 2 );
    colorsLayout->addWidget( printBorderColorBox, row, COL_PRT_CHANNEL, 1, 2 );
    ++row;
    colorsLayout->addWidget( markersLabel, row, COL_LABEL );
    colorsLayout->addWidget( markersColorBox, row, COL_SCR_CHANNEL, 1, 2 );
    colorsLayout->addWidget( printMarkersColorBox, row, COL_PRT_CHANNEL, 1, 2 );
    ++row;
    colorsLayout->addWidget( textLabel, row, COL_LABEL );
    colorsLayout->addWidget( textColorBox, row, COL_SCR_CHANNEL, 1, 2 );
    colorsLayout->addWidget( printTextColorBox, row, COL_PRT_CHANNEL, 1, 2 );
    ++row;

    // horizontal line
    horizontalLine = new QLabel( "<hr width=\"100%\"/>" );
    horizontalLine->setAlignment( Qt::AlignRight );
    horizontalLine->setTextFormat( Qt::RichText );
    colorsLayout->addWidget( horizontalLine, row, COL_LABEL, 1, COL_PRT_SPECTRUM - COL_LABEL + 1 );
    ++row;

    colorsLayout->addWidget( screenChannelLabel, row, COL_SCR_CHANNEL );
    colorsLayout->addWidget( screenSpectrumLabel, row, COL_SCR_SPECTRUM );
    colorsLayout->addWidget( printChannelLabel, row, COL_PRT_CHANNEL );
    colorsLayout->addWidget( printSpectrumLabel, row, COL_PRT_SPECTRUM );
    ++row;

    for ( ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel, ++row ) {
        colorsLayout->addWidget( colorLabel[ channel ], row, COL_LABEL );
        colorsLayout->addWidget( screenChannelColorBox[ channel ], row, COL_SCR_CHANNEL );
        colorsLayout->addWidget( screenSpectrumColorBox[ channel ], row, COL_SCR_SPECTRUM );
        colorsLayout->addWidget( printChannelColorBox[ channel ], row, COL_PRT_CHANNEL );
        colorsLayout->addWidget( printSpectrumColorBox[ channel ], row, COL_PRT_SPECTRUM );
    }

    // horizontal line
    horizontalLine = new QLabel( "<hr width=\"100%\"/>" );
    horizontalLine->setAlignment( Qt::AlignRight );
    horizontalLine->setTextFormat( Qt::RichText );

    colorsLayout->addWidget( horizontalLine, ++row, COL_LABEL, 1, COL_PRT_SPECTRUM - COL_LABEL + 1 );

    styleFusionCheckBox = new QCheckBox( tr( "Fusion style (restart needed to apply the change)" ) );
    styleFusionCheckBox->setChecked( settings->view.styleFusion );
    colorsLayout->addWidget( styleFusionCheckBox, ++row, COL_LABEL );

    screenColorCheckBox = new QCheckBox( tr( "Export hardcopy images or pdf files with printer colors" ) );
    screenColorCheckBox->setChecked( settings->view.printerColorImages );
    colorsLayout->addWidget( screenColorCheckBox, row, COL_SCR_CHANNEL, 1, COL_PRT_SPECTRUM - COL_SCR_CHANNEL + 1 );

    fontSizeLabel = new QLabel( tr( "Font size (restart needed to apply the change)" ) );
    fontSizeSpinBox = new QSpinBox();
    fontSizeSpinBox->setMinimum( 6 );
    fontSizeSpinBox->setMaximum( 24 );
    fontSizeSpinBox->setValue( settings->view.fontSize );
    colorsLayout->addWidget( fontSizeLabel, ++row, COL_LABEL );
    colorsLayout->addWidget( fontSizeSpinBox, row, COL_LABEL + 1 );

    QStringList themeStrings;
    themeStrings << tr( "Auto" ) << tr( "Light" ) << tr( "Dark" );
    themeLabel = new QLabel( tr( "Theme (restart needed to apply the change)" ) );
    themeComboBox = new QComboBox();
    themeComboBox->addItems( themeStrings );
    themeComboBox->setCurrentIndex( settings->view.theme );
    colorsLayout->addWidget( themeLabel, ++row, COL_LABEL );
    colorsLayout->addWidget( themeComboBox, row, COL_LABEL + 1 );

    colorsGroup = new QGroupBox( tr( "Screen and Print Colors" ) );
    colorsGroup->setLayout( colorsLayout );

    // Main layout
    mainLayout = new QVBoxLayout();
    mainLayout->addWidget( colorsGroup );
    mainLayout->addStretch( 1 );

    setLayout( mainLayout );
}

/// \brief Saves the new settings.
void DsoConfigColorsPage::saveSettings() {
    DsoSettingsView &colorSettings = settings->view;

    // Screen category
    colorSettings.screen.axes = axesColorBox->getColor();
    colorSettings.screen.background = backgroundColorBox->getColor();
    colorSettings.screen.border = borderColorBox->getColor();
    colorSettings.screen.grid = gridColorBox->getColor();
    colorSettings.screen.markers = markersColorBox->getColor();
    colorSettings.screen.text = textColorBox->getColor();

    // Print category
    colorSettings.print.axes = printAxesColorBox->getColor();
    colorSettings.print.background = printBackgroundColorBox->getColor();
    colorSettings.print.border = printBorderColorBox->getColor();
    colorSettings.print.grid = printGridColorBox->getColor();
    colorSettings.print.markers = printMarkersColorBox->getColor();
    colorSettings.print.text = printTextColorBox->getColor();

    // Graph category
    for ( ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel ) {
        colorSettings.screen.voltage[ channel ] = screenChannelColorBox[ channel ]->getColor();
        colorSettings.screen.spectrum[ channel ] = screenSpectrumColorBox[ channel ]->getColor();
        colorSettings.print.voltage[ channel ] = printChannelColorBox[ channel ]->getColor();
        colorSettings.print.spectrum[ channel ] = printSpectrumColorBox[ channel ]->getColor();
    }
    settings->view.printerColorImages = screenColorCheckBox->isChecked();
    settings->view.fontSize = fontSizeSpinBox->value();

    settings->view.styleFusion = styleFusionCheckBox->isChecked();
    QSettings().setValue( "view/styleFusion", settings->view.styleFusion );
    settings->view.theme = themeComboBox->currentIndex();
    QSettings().setValue( "view/theme", settings->view.theme );
}
