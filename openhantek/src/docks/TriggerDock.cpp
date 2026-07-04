// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
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


TriggerDock::TriggerDock( DsoSettingsScope *scope, const DsoSettingsView *view, const Dso::ControlSpecification *spec,
                          QWidget *parent )
    : QDockWidget( tr( "Trigger" ), parent ), scope( scope ), mSpec( spec ) {

    if ( scope->verboseLevel > 1 )
        qDebug() << " TriggerDock::TriggerDock()";

    // Initialize lists for the panel keys
    for ( ChannelID channel = 0; channel < mSpec->channels; ++channel )
        sourceStandardStrings << tr( "CH%1" ).arg( channel + 1 );
    sourceStandardStrings << tr( "MATH" );
    // add "smooth" source
    smoothStandardStrings << tr( "HF" ) << tr( "Normal" ) << tr( "LF" );

    // Initialize elements
    modeLabel = new QLabel( tr( "Mode" ) );
    modeGroup = new ScopeButtonGroup();
    if ( scope->toolTipVisible )
        modeGroup->setToolTip( tr( "Select the trigger mode" ) );
    for ( Dso::TriggerMode mode : mSpec->triggerModes )
        modeGroup->addItem( Dso::triggerModeString( mode ) );

    slopeLabel = new QLabel( tr( "Slope" ) );
    slopeGroup = new ScopeButtonGroup();
    if ( scope->toolTipVisible )
        slopeGroup->setToolTip( tr( "Select positive, negative or both (alternating) slopes" ) );
    for ( Dso::Slope slope : Dso::SlopeEnum )
        slopeGroup->addItem( Dso::slopeString( slope ) );

    sourceLabel = new QLabel( tr( "Source" ) );
    sourceGroup = new ScopeButtonGroup();
    if ( scope->toolTipVisible )
        sourceGroup->setToolTip( tr( "Select the trigger channel (CH1, CH2, or MATH)" ) );
    for ( int source = 0; source < sourceStandardStrings.size(); ++source ) // light up in the channel trace color
        sourceGroup->addItem( sourceStandardStrings[ source ],
                              unsigned( source ) < view->screen.voltage.size() ? view->screen.voltage[ unsigned( source ) ]
                                                                               : QColor() );
    smoothButton = new ScopeCycleButton();
    if ( scope->toolTipVisible )
        smoothButton->setToolTip( tr( "Trigger on fast, normal, or slow signals - click to cycle" ) );
    smoothButton->addItems( smoothStandardStrings );

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth( 0, 50 );
    dockLayout->setColumnStretch( 1, 1 ); // stretch 2nd (middle) column 1x
    dockLayout->setColumnStretch( 2, 2 ); // stretch 3rd (last) column 2x
    dockLayout->setSpacing( DOCK_LAYOUT_SPACING );
    dockLayout->addWidget( modeLabel, 0, 0 );
    dockLayout->addWidget( modeGroup, 0, 1, 1, 2 ); // fill 1 row, 2 col
    dockLayout->addWidget( sourceLabel, 1, 0 );
    dockLayout->addWidget( sourceGroup, 1, 1, 1, 2 ); // fill 1 row, 2 col
    dockLayout->addWidget( slopeLabel, 2, 0 );
    dockLayout->addWidget( slopeGroup, 2, 1 );
    dockLayout->addWidget( smoothButton, 2, 2 );

    dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );

    // Load settings into GUI
    loadSettings( scope );

    // Connect signals and slots
    connect( modeGroup, &ScopeButtonGroup::currentIndexChanged, this, [ this ]( int index ) {
        this->scope->trigger.mode = mSpec->triggerModes[ unsigned( index ) ];
        emit modeChanged( this->scope->trigger.mode );
    } );
    connect( slopeGroup, &ScopeButtonGroup::currentIndexChanged, this, [ this ]( int index ) {
        this->scope->trigger.slope = Dso::Slope( index );
        emit slopeChanged( this->scope->trigger.slope );
    } );
    connect( sourceGroup, &ScopeButtonGroup::currentIndexChanged, this, [ this ]( int index ) {
        this->scope->trigger.source = index;
        emit sourceChanged( index );
    } );
    connect( smoothButton, &ScopeCycleButton::currentIndexChanged, this, [ this ]( int index ) {
        this->scope->trigger.smooth = index;
        emit smoothChanged( index );
    } );
}

void TriggerDock::loadSettings( DsoSettingsScope *scope ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  TDock::loadSettings()";
    // Set values
    if ( scope->trigger.mode != Dso::TriggerMode::ROLL && scope->horizontal.timebase < 0.2 ) // disable ROLL mode
        modeGroup->setItemEnabled( int( mSpec->triggerModes.size() ) - 1, false );
    setMode( scope->trigger.mode );
    setSlope( scope->trigger.slope );
    setSource( scope->trigger.source );
    setSmooth( scope->trigger.smooth );
}


void TriggerDock::timebaseChanged( double timebase ) { // provide ROLL mode only if samplerate > 100 ms/div
    if ( scope->trigger.mode == Dso::TriggerMode::ROLL )
        return;
    modeGroup->setItemEnabled( int( mSpec->triggerModes.size() ) - 1, timebase > 0.1 );
}


/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void TriggerDock::closeEvent( QCloseEvent *event ) {
    hide();

    event->accept();
}

void TriggerDock::setMode( Dso::TriggerMode mode ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  TDock::setMode()" << int( mode );
    int index = int( std::find( mSpec->triggerModes.begin(), mSpec->triggerModes.end(), mode ) - mSpec->triggerModes.begin() );
    modeGroup->setCurrentIndex( index ); // does not emit
    emit modeChanged( scope->trigger.mode );
}

void TriggerDock::setSlope( Dso::Slope slope ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  TDock::setSlope()" << int( slope );
    slopeGroup->setCurrentIndex( int( slope ) ); // does not emit
}

void TriggerDock::setSource( int id ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  TDock::setSource()" << id;
    if ( id >= sourceStandardStrings.count() )
        return;
    sourceGroup->setCurrentIndex( id ); // does not emit
}

void TriggerDock::setSmooth( int smooth ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  TDock::setSmooth()" << smooth;
    if ( int( smooth ) >= smoothStandardStrings.count() )
        return;
    smoothButton->setCurrentIndex( int( smooth ) ); // does not emit
}
