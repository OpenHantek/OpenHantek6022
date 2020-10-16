// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "TriggerDock.h"
#include "dockwindows.h"

#include "dsosettings.h"
#include "hantekdso/controlspecification.h"
#include "sispinbox.h"
#include "utils/printutils.h"


TriggerDock::TriggerDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent )
    : QDockWidget( tr( "Trigger" ), parent ), scope( scope ), mSpec( spec ) {

    // Initialize lists for comboboxes
    for ( ChannelID channel = 0; channel < mSpec->channels; ++channel )
        this->sourceStandardStrings << tr( "CH%1" ).arg( channel + 1 );
    // add "smooth" source
    for ( ChannelID channel = 0; channel < mSpec->channels; ++channel )
        this->sourceStandardStrings << tr( "CH%1 smooth" ).arg( channel + 1 );

    // Initialize elements
    modeLabel = new QLabel( tr( "Mode" ) );
    modeComboBox = new QComboBox();
    for ( Dso::TriggerMode mode : mSpec->triggerModes )
        modeComboBox->addItem( Dso::triggerModeString( mode ) );

    slopeLabel = new QLabel( tr( "Slope" ) );
    slopeComboBox = new QComboBox();
    for ( Dso::Slope slope : Dso::SlopeEnum )
        slopeComboBox->addItem( Dso::slopeString( slope ) );

    sourceLabel = new QLabel( tr( "Source" ) );
    sourceComboBox = new QComboBox();
    sourceComboBox->addItems( sourceStandardStrings );

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth( 0, 64 );
    dockLayout->setColumnStretch( 1, 1 );
    dockLayout->setSpacing( DOCK_LAYOUT_SPACING );
    dockLayout->addWidget( modeLabel, 0, 0 );
    dockLayout->addWidget( modeComboBox, 0, 1 );
    dockLayout->addWidget( sourceLabel, 1, 0 );
    dockLayout->addWidget( sourceComboBox, 1, 1 );
    dockLayout->addWidget( slopeLabel, 2, 0 );
    dockLayout->addWidget( slopeComboBox, 2, 1 );

    dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );

    // Load settings into GUI
    loadSettings( scope );

    // Connect signals and slots
    connect( modeComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ), [this]( int index ) {
        this->scope->trigger.mode = mSpec->triggerModes[ unsigned( index ) ];
        emit modeChanged( this->scope->trigger.mode );
    } );
    connect( slopeComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ), [this]( int index ) {
        this->scope->trigger.slope = Dso::Slope( index );
        emit slopeChanged( this->scope->trigger.slope );
    } );
    connect( sourceComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ),
             [this]( unsigned index ) {
                 bool smooth = index >= mSpec->channels;
                 this->scope->trigger.smooth = smooth;
                 this->scope->trigger.source = index & ( mSpec->channels - 1 );
                 emit sourceChanged( index & ( mSpec->channels - 1 ), smooth );
             } );
}

void TriggerDock::loadSettings( DsoSettingsScope *scope ) {
    // Set values
    setMode( scope->trigger.mode );
    setSlope( scope->trigger.slope );
    setSource( int( scope->trigger.source ), scope->trigger.smooth );
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void TriggerDock::closeEvent( QCloseEvent *event ) {
    this->hide();

    event->accept();
}

void TriggerDock::setMode( Dso::TriggerMode mode ) {
    int index = int( std::find( mSpec->triggerModes.begin(), mSpec->triggerModes.end(), mode ) - mSpec->triggerModes.begin() );
    QSignalBlocker blocker( modeComboBox );
    modeComboBox->setCurrentIndex( index );
    emit modeChanged( this->scope->trigger.mode );
}

void TriggerDock::setSlope( Dso::Slope slope ) {
    QSignalBlocker blocker( slopeComboBox );
    slopeComboBox->setCurrentIndex( int( slope ) );
}

void TriggerDock::setSource( int id, bool smooth ) {
    if ( smooth )
        id += mSpec->channels;
    if ( id >= this->sourceStandardStrings.count() )
        return;
    QSignalBlocker blocker( sourceComboBox );
    sourceComboBox->setCurrentIndex( id );
}
