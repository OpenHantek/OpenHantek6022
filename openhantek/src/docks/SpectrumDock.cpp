// SPDX-License-Identifier: GPL-2.0+

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
#include "viewconstants.h"

template < typename... Args > struct SELECT {
    template < typename C, typename R > static constexpr auto OVERLOAD_OF( R ( C::*pmf )( Args... ) ) -> decltype( pmf ) {
        return pmf;
    }
};

SpectrumDock::SpectrumDock( DsoSettingsScope *scope, QWidget *parent, Qt::WindowFlags flags )
    : QDockWidget( tr( "Spectrum" ), parent, flags ), scope( scope ) {

    // Initialize lists for comboboxes
    this->magnitudeSteps = {1, 2, 3, 6, 10, 20, 40, 60, 80, 100};
    for ( const auto &magnitude : magnitudeSteps )
        this->magnitudeStrings << valueToString( magnitude, UNIT_DECIBEL, 0 );

    this->dockLayout = new QGridLayout();
    this->dockLayout->setColumnMinimumWidth( 0, 64 );
    this->dockLayout->setColumnStretch( 1, 1 );
    this->dockLayout->setSpacing( DOCK_LAYOUT_SPACING );

    // Initialize elements
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        ChannelBlock b;
        b.magnitudeComboBox = ( new QComboBox() );
        QString name = scope->spectrum[ channel ].name;
        name.insert( int( channel ), '&' ); // &SP1, S&P2, SP&M
        b.usedCheckBox = ( new QCheckBox( name ) );

        channelBlocks.push_back( b );

        this->dockLayout->addWidget( b.usedCheckBox, int( channel ), 0 );
        this->dockLayout->addWidget( b.magnitudeComboBox, int( channel ), 1 );

        b.magnitudeComboBox->addItems( this->magnitudeStrings );

        // Connect signals and slots
        connect( b.usedCheckBox, &QCheckBox::toggled, [this, channel]( bool checked ) {
            // Send signal if it was one of the checkboxes
            if ( channel < this->scope->voltage.size() ) {
                this->scope->spectrum[ channel ].used = checked;
                emit usedChanged( channel, checked );
            }
        } );

        connect( b.magnitudeComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ),
                 [this, channel]( unsigned index ) {
                     // Send signal if it was one of the comboboxes
                     if ( channel < this->scope->voltage.size() ) {
                         this->scope->spectrum[ channel ].magnitude = this->magnitudeSteps.at( index );
                         emit magnitudeChanged( channel, this->scope->spectrum[ channel ].magnitude );
                     }
                 } );
    }

    // Load settings into GUI
    this->loadSettings( scope );

    dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );
}

void SpectrumDock::loadSettings( DsoSettingsScope *scope ) {
    // Initialize elements
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        this->setMagnitude( channel, scope->spectrum[ channel ].magnitude );
        this->setUsed( channel, scope->spectrum[ channel ].used );
    }
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void SpectrumDock::closeEvent( QCloseEvent *event ) {
    this->hide();

    event->accept();
}

int SpectrumDock::setMagnitude( ChannelID channel, double magnitude ) {
    if ( channel >= scope->voltage.size() )
        return -1;
    QSignalBlocker blocker( channelBlocks[ channel ].magnitudeComboBox );

    auto indexIt = std::find( magnitudeSteps.begin(), magnitudeSteps.end(), magnitude );
    if ( indexIt == magnitudeSteps.end() )
        return -1;
    int index = int( std::distance( magnitudeSteps.begin(), indexIt ) );
    channelBlocks[ channel ].magnitudeComboBox->setCurrentIndex( index );
    return index;
}

unsigned SpectrumDock::setUsed( ChannelID channel, bool used ) {
    if ( channel >= scope->voltage.size() )
        return INT_MAX;
    QSignalBlocker blocker( channelBlocks[ channel ].usedCheckBox );

    channelBlocks[ channel ].usedCheckBox->setChecked( used );
    return channel;
}
