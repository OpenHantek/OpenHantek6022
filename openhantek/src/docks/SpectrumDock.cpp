// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "SpectrumDock.h"
#include "dockwindows.h"

#include "dsosettings.h"
#include "sispinbox.h"
#include "utils/printutils.h"


template < typename... Args > struct SELECT {
    template < typename C, typename R > static constexpr auto OVERLOAD_OF( R ( C::*pmf )( Args... ) ) -> decltype( pmf ) {
        return pmf;
    }
};


SpectrumDock::SpectrumDock( DsoSettingsScope *scope, QWidget *parent ) : QDockWidget( tr( "Spectrum" ), parent ), scope( scope ) {

    if ( scope->verboseLevel > 1 )
        qDebug() << " SpectrumDock::SpectrumDock()";

    // Initialize lists for comboboxes
    magnitudeSteps = { 1, 2, 3, 6, 10, 20, 40, 60, 80, 100 };
    for ( const auto &magnitude : magnitudeSteps )
        magnitudeStrings << valueToString( magnitude, UNIT_DECIBEL, 0 );

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth( 0, 64 );
    dockLayout->setColumnStretch( 1, 1 );
    dockLayout->setSpacing( DOCK_LAYOUT_SPACING );

    // Initialize elements
    unsigned channel;
    for ( channel = 0; channel < scope->voltage.size(); ++channel ) {
        ChannelBlock b;
        b.magnitudeComboBox = ( new QComboBox() );
        if ( scope->toolTipVisible )
            b.magnitudeComboBox->setToolTip( tr( "Magnitude per vertical screen division" ) );
        QString name = scope->spectrum[ channel ].name;
        name.insert( int( channel ), '&' ); // &SP1, S&P2, SP&M
        b.usedCheckBox = ( new QCheckBox( name ) );

        channelBlocks.push_back( b );

        dockLayout->addWidget( b.usedCheckBox, int( channel ), 0 );
        dockLayout->addWidget( b.magnitudeComboBox, int( channel ), 1 );

        b.magnitudeComboBox->addItems( magnitudeStrings );

        // Connect signals and slots
        connect( b.usedCheckBox, &QCheckBox::toggled, this, [ this, channel ]( bool checked ) {
            if ( channel < this->scope->voltage.size() ) {
                setUsed( channel, checked );
            }
        } );

        connect( b.magnitudeComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
                 [ this, channel ]( unsigned index ) {
                     if ( channel < this->scope->voltage.size() ) {
                         this->setMagnitude( channel, this->magnitudeSteps.at( index ) );
                     }
                 } );
    }
    frequencybaseLabel = new QLabel( tr( "Frequencybase" ) );
    frequencybaseSiSpinBox = new SiSpinBox( UNIT_HERTZ );
    if ( scope->toolTipVisible )
        frequencybaseSiSpinBox->setToolTip( tr( "Frequency range per horizontal screen division" ) );
    frequencybaseSiSpinBox->setMinimum( 0.1 );
    frequencybaseSiSpinBox->setMaximum( 100e6 );
    dockLayout->addWidget( frequencybaseLabel, int( channel ), 0 );
    dockLayout->addWidget( frequencybaseSiSpinBox, int( channel ), 1 );
    connect( frequencybaseSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             [ this ]() { this->frequencybaseSelected( this->frequencybaseSiSpinBox->value() ); } );

    // Load settings into GUI
    loadSettings( scope );

    dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );
}


void SpectrumDock::loadSettings( DsoSettingsScope *scope ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  SDock::loadSettings()";
    // Initialize elements
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        setMagnitude( channel, scope->spectrum[ channel ].magnitude );
        setUsed( channel, scope->spectrum[ channel ].used );
        scope->spectrum[ channel ].visible = scope->spectrum[ channel ].used;
        channelBlocks[ channel ].usedCheckBox->setEnabled( scope->horizontal.format == Dso::GraphFormat::TY );
    }
    setFrequencybase( scope->horizontal.frequencybase );
}


/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void SpectrumDock::closeEvent( QCloseEvent *event ) {
    hide();
    event->accept();
}


int SpectrumDock::setMagnitude( ChannelID channel, double magnitude ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  SDock::setMagnitude()" << channel << magnitude;
    if ( channel >= scope->voltage.size() )
        return -1;
    QSignalBlocker blocker( channelBlocks[ channel ].magnitudeComboBox );

    auto indexIt = std::find( magnitudeSteps.begin(), magnitudeSteps.end(), magnitude );
    if ( indexIt == magnitudeSteps.end() )
        return -1;
    int index = int( std::distance( magnitudeSteps.begin(), indexIt ) );
    channelBlocks[ channel ].magnitudeComboBox->setCurrentIndex( index );
    scope->spectrum[ channel ].magnitude = magnitude;
    emit magnitudeChanged( channel, scope->spectrum[ channel ].magnitude );
    return index;
}


unsigned SpectrumDock::setUsed( ChannelID channel, bool used ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  SDock::setUsed()" << channel << used;
    if ( channel >= scope->voltage.size() )
        return INT_MAX;
    scope->spectrum[ channel ].used = used;
    scope->spectrum[ channel ].visible = used;
    QSignalBlocker blocker( channelBlocks[ channel ].usedCheckBox );
    channelBlocks[ channel ].usedCheckBox->setChecked( used );
    if ( used )
        emit usedChannelChanged( channel, channel + 1 ); // channel bit mask 0b01, 0b10, 0b11
    else
        emit usedChannelChanged( channel, 0 );
    return channel;
}


void SpectrumDock::enableSpectrumDock( bool enabled ) { // disable when using XY display
    if ( scope->verboseLevel > 2 )
        qDebug() << "  SDock::enableSpectrum()" << enabled;
    for ( unsigned channel = 0; channel < scope->voltage.size(); ++channel ) {
        QSignalBlocker blocker( channelBlocks[ channel ].usedCheckBox );
        channelBlocks[ channel ].usedCheckBox->setEnabled( enabled );
        channelBlocks[ channel ].usedCheckBox->setChecked( false );
        scope->spectrum[ channel ].used = false;
        emit usedChannelChanged( channel, 0 );
    }
}


/// \brief Called when the samplerate from horizontal dock changes its value.
/// \param samplerare The samplerate in hertz.
void SpectrumDock::setSamplerate( double samplerate ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  SDock::setSamplerate()" << samplerate;
    double maxFreqBase = samplerate / DIVS_TIME / 2; // Nyquist frequency
    frequencybaseSiSpinBox->setMaximum( maxFreqBase );
    if ( frequencybaseSiSpinBox->value() > maxFreqBase )
        setFrequencybase( maxFreqBase );
}


void SpectrumDock::setFrequencybase( double frequencybase ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  SDock::setFrequencybase()" << frequencybase;
    QSignalBlocker blocker( frequencybaseSiSpinBox );
    frequencybaseSiSpinBox->setValue( frequencybase );
}


/// \brief Called when the frequencybase spinbox changes its value.
/// \param frequencybase The frequencybase in hertz.
void SpectrumDock::frequencybaseSelected( double frequencybase ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  SDock::frequencybaseSelected()" << frequencybase;
    // printf( "SD::frequencybaseSelected( %g )\n", frequencybase );
    scope->horizontal.frequencybase = frequencybase;
    emit frequencybaseChanged( frequencybase );
}
