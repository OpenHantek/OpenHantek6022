// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCloseEvent>
#include <QDebug>
#include <QDockWidget>
#include <QSignalBlocker>

#include <cmath>

#include "VoltageDock.h"
#include "dockwindows.h"
#include "utils/printutils.h"


template < typename... Args > struct SELECT {
    template < typename C, typename R > static constexpr auto OVERLOAD_OF( R ( C::*pmf )( Args... ) ) -> decltype( pmf ) {
        return pmf;
    }
};


VoltageDock::VoltageDock( DsoSettingsScope *scope, const DsoSettingsView *view, const Dso::ControlSpecification *spec,
                          QWidget *parent )
    : QDockWidget( tr( "Voltage" ), parent ), scope( scope ), spec( spec ) {

    if ( scope->verboseLevel > 1 )
        qDebug() << " VoltageDock::VoltageDock()";

    // Initialize lists for comboboxes
    for ( Dso::Coupling c : spec->couplings )
        if ( c == Dso::Coupling::DC || scope->hasACcoupling || scope->hasACmodification )
            couplingStrings.append( Dso::couplingString( c ) );

    for ( auto e : Dso::MathModeEnum ) {
        modeStrings.append( Dso::mathModeString( e ) );
    }

    updateGainStrings();
    for ( double mathGainStep : scope->mathGainSteps ) {
        mathGainStrings << valueToString( mathGainStep, UNIT_VOLTS, 0 );
    }

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth( 0, 50 );
    dockLayout->setColumnStretch( 1, 1 ); // stretch ComboBox in 2nd (middle) column
    dockLayout->setColumnStretch( 2, 1 ); // stretch ComboBox in 3rd (last) column
    dockLayout->setSpacing( DOCK_LAYOUT_SPACING );
    // Initialize elements
    int row = 0;
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        ChannelBlock b;

        QColor channelColor = channel < view->screen.voltage.size() ? view->screen.voltage[ channel ] : QColor();
        if ( channel < spec->channels )
            b.usedCheckBox = new ScopeButton( tr( "CH&%1" ).arg( channel + 1 ) ); // define shortcut <ALT>1 / <ALT>2
        else
            b.usedCheckBox = new ScopeButton( tr( "MA&TH" ) );
        if ( channelColor.isValid() ) // light up the key in the channel trace color
            b.usedCheckBox->setAccentColor( channelColor );
        b.miscComboBox = new QComboBox();
        b.couplingGroup = new ScopeButtonGroup();
        b.couplingGroup->addItems( couplingStrings );
        if ( couplingStrings.size() < 2 ) { // only DC available -> show it lit but not clickable
            b.couplingGroup->setItemEnabled( 0, false );
            b.couplingGroup->setToolTip( tr( "AC coupling needs a hardware modification, see Help menu" ) );
        }
        // scale "knob": big -/+ keys and a scope style V/div readout in the channel color
        b.gainLabel = new QLabel();
        b.gainLabel->setAlignment( Qt::AlignCenter );
        QFont gainFont = b.gainLabel->font();
        gainFont.setPointSizeF( gainFont.pointSizeF() * 1.25 );
        gainFont.setBold( true );
        b.gainLabel->setFont( gainFont );
        if ( channelColor.isValid() )
            b.gainLabel->setStyleSheet( QString( "color: %1;" ).arg( channelColor.name() ) );
        if ( scope->toolTipVisible )
            b.gainLabel->setToolTip( tr( "Voltage range per vertical screen division" ) );
        b.scaleKnob = new ScopeKnob();
        if ( channelColor.isValid() )
            b.scaleKnob->setAccentColor( channelColor );
        if ( scope->toolTipVisible )
            b.scaleKnob->setToolTip( tr( "Scale knob: turn (drag or scroll) clockwise to zoom in" ) );
        b.invertCheckBox = new ScopeButton( tr( "Invert" ) );
        b.attnButton = new ScopeCycleButton();
        b.attnButton->addItems( { tr( "x1" ), tr( "x10" ), tr( "x100" ) } );
        if ( scope->toolTipVisible )
            b.attnButton->setToolTip( tr( "Probe attenuation - click to cycle x1, x10, x100" ) );

        channelBlocks.push_back( std::move( b ) );

        if ( channel < spec->channels ) {
            if ( scope->toolTipVisible )
                b.couplingGroup->setToolTip( tr( "Select DC or AC coupling" ) );
        } else {
            b.miscComboBox->addItems( modeStrings );
            if ( scope->toolTipVisible )
                b.miscComboBox->setToolTip( tr( "Select the mathematical operation for this channel" ) );
        }

        // one front panel "card" per channel, like the vertical section of a HW scope
        QFrame *card = new QFrame();
        card->setObjectName( "channelCard" );
        QGridLayout *cardLayout = new QGridLayout( card );
        cardLayout->setContentsMargins( 4, 4, 4, 4 );
        cardLayout->setSpacing( 2 );
        cardLayout->setColumnStretch( 0, 1 );
        cardLayout->setColumnStretch( 1, 1 );
        if ( channel < spec->channels ) {
            // [CH key] [V/div readout] [SCALE knob spanning both rows]
            cardLayout->addWidget( b.usedCheckBox, 0, 0 );
            cardLayout->addWidget( b.gainLabel, 0, 1 );
            cardLayout->addWidget( b.scaleKnob, 0, 2, 2, 1, Qt::AlignCenter );
            // [coupling] [invert + probe]
            cardLayout->addWidget( b.couplingGroup, 1, 0 );
            QHBoxLayout *miscLayout = new QHBoxLayout();
            miscLayout->setSpacing( 2 );
            miscLayout->addWidget( b.invertCheckBox );
            miscLayout->addWidget( b.attnButton );
            cardLayout->addLayout( miscLayout, 1, 1 );
        } else { // MATH channel
            cardLayout->addWidget( b.usedCheckBox, 0, 0 );
            cardLayout->addWidget( b.miscComboBox, 0, 1 );
            cardLayout->addWidget( b.scaleKnob, 0, 2, 2, 1, Qt::AlignCenter );
            cardLayout->addWidget( b.gainLabel, 1, 0, 1, 2 );
        }
        dockLayout->addWidget( card, row++, 0, 1, 3 );

        auto stepGain = [ this, channel, spec ]( int delta ) { // the scale "knob" was turned
            DsoSettingsScopeVoltage &voltage = this->scope->voltage[ channel ];
            const int count = channel < spec->channels ? int( this->scope->gainSteps.size() )
                                                       : int( this->scope->mathGainSteps.size() );
            const int index = qBound( 0, int( voltage.gainStepIndex ) + delta, count - 1 );
            if ( index == int( voltage.gainStepIndex ) )
                return;
            voltage.gainStepIndex = unsigned( index );
            setGain( channel, unsigned( index ) );
            emit gainChanged( channel, this->scope->gain( channel ) );
        };
        // clockwise (positive detents) zooms in -> smaller V/div, like on a HW scope
        connect( b.scaleKnob, &ScopeKnob::stepped, this, [ stepGain ]( int delta ) { stepGain( -delta ); } );
        connect( b.attnButton, &ScopeCycleButton::currentIndexChanged, this, [ this, channel ]( int index ) {
            const double attnValue = pow( 10.0, index ); // x1, x10, x100
            this->scope->voltage[ channel ].probeAttn = attnValue;
            setAttn( channel, attnValue );
            emit probeAttnChanged( channel, attnValue ); // make sure to set the probe first, since this will influence the gain
            emit gainChanged( channel, this->scope->gain( channel ) );
        } );
        connect( b.invertCheckBox, &QAbstractButton::clicked, this, [ this, channel ]( bool checked ) {
            this->scope->voltage[ channel ].inverted = checked;
            emit invertedChanged( channel, checked );
        } );
        connect( b.couplingGroup, &ScopeButtonGroup::currentIndexChanged, this,
                 [ this, channel, spec, scope ]( int index ) {
                     if ( channel >= spec->channels )
                         return;
                     this->scope->voltage[ channel ].couplingOrMathIndex = unsigned( index );
                     emit couplingChanged( channel, scope->coupling( channel, spec ) );
                 } );
        connect( b.miscComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
                 [ this, channel, spec ]( unsigned index ) {
                     if ( channel < spec->channels ) // CH1 & CH2 use the coupling button group instead
                         return;
                     this->scope->voltage[ channel ].couplingOrMathIndex = index;
                     Dso::MathMode mathMode = Dso::getMathMode( this->scope->voltage[ channel ] );
                     setAttn( channel, this->scope->voltage[ channel ].probeAttn );
                     emit modeChanged( mathMode );
                     emit usedChannelChanged( channel, Dso::mathChannelsUsed( mathMode ) );
                 } );
        connect( b.usedCheckBox, &QAbstractButton::clicked, this, [ this, channel ]( bool checked ) {
            this->scope->voltage[ channel ].used = checked;
            this->scope->voltage[ channel ].visible = checked;
            unsigned mask = 0;
            if ( checked ) {
                if ( channel < this->spec->channels )
                    mask = channel + 1;
                else
                    mask = Dso::mathChannelsUsed( Dso::MathMode( this->scope->voltage[ 2 ].couplingOrMathIndex ) );
            }
            emit usedChannelChanged( channel, mask ); // channel bit mask 0b01, 0b10, 0b11
        } );
    }

    // Load settings into GUI
    loadSettings( scope, spec );

    dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );
}


void VoltageDock::loadSettings( DsoSettingsScope *scope, const Dso::ControlSpecification *spec ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::loadSettings()";
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        if ( channel < spec->channels ) {
            if ( int( scope->voltage[ channel ].couplingOrMathIndex ) < couplingStrings.size() )
                setCoupling( channel, scope->voltage[ channel ].couplingOrMathIndex );
        } else {
            setMode( scope->voltage[ channel ].couplingOrMathIndex );
        }

        setGain( channel, scope->voltage[ channel ].gainStepIndex );
        setUsed( channel, scope->voltage[ channel ].used );
        scope->voltage[ channel ].visible = scope->voltage[ channel ].used;
        setAttn( channel, scope->voltage[ channel ].probeAttn );
        setInverted( channel, scope->voltage[ channel ].inverted );
    }
}


/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void VoltageDock::closeEvent( QCloseEvent *event ) {
    hide();
    event->accept();
}


void VoltageDock::setCoupling( ChannelID channel, unsigned couplingIndex ) {
    if ( channel >= spec->channels )
        return;
    if ( couplingIndex >= spec->couplings.size() )
        return;
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::setCoupling()" << channel << couplingStrings[ int( couplingIndex ) ];
    channelBlocks[ channel ].couplingGroup->setCurrentIndex( int( couplingIndex ) ); // does not emit
}


void VoltageDock::setGain( ChannelID channel, unsigned gainStepIndex ) {
    if ( channel >= scope->voltage.size() )
        return;
    if ( channel < spec->channels ) { // Voltage channel
        if ( gainStepIndex >= scope->gainSteps.size() )
            return;
        if ( scope->verboseLevel > 2 )
            qDebug() << "  VDock::setGain()" << channel << gainStrings[ int( gainStepIndex ) ];
        channelBlocks[ channel ].gainLabel->setText( gainStrings[ int( gainStepIndex ) ] + tr( "/div" ) );
    } else {
        if ( gainStepIndex >= scope->mathGainSteps.size() )
            return;
        if ( scope->verboseLevel > 2 )
            qDebug() << "  VDock::setGain()" << channel << mathGainStrings[ int( gainStepIndex ) ];
        channelBlocks[ channel ].gainLabel->setText( mathGainStrings[ int( gainStepIndex ) ] + tr( "/div" ) );
    }
}


void VoltageDock::setAttn( ChannelID channel, double attnValue ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::setAttn()" << channel << attnValue;
    if ( channel >= scope->voltage.size() )
        return;
    // change unit to V² for the multiplying math functions
    if ( channel < spec->channels ) { // Voltage channel
        updateGainStrings( attnValue );
    } else {
        mathGainStrings.clear();
        for ( double mathGainStep : scope->mathGainSteps )
            mathGainStrings << valueToString(
                mathGainStep * attnValue,
                Dso::mathModeUnit( Dso::MathMode( scope->voltage[ spec->channels ].couplingOrMathIndex ) ),
                -1 ); // auto format V or V²
    }
    scope->voltage[ channel ].probeAttn = attnValue;
    setGain( channel, scope->voltage[ channel ].gainStepIndex ); // refresh the V/div readout
    // snap the display to the nearest probe step x1 / x10 / x100
    channelBlocks[ channel ].attnButton->setCurrentIndex( qBound( 0, int( round( log10( attnValue ) ) ), 2 ) );
}


void VoltageDock::setMode( unsigned mathModeIndex ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::setMode()" << modeStrings[ int( mathModeIndex ) ];
    QSignalBlocker blocker( channelBlocks[ spec->channels ].miscComboBox );
    channelBlocks[ spec->channels ].miscComboBox->setCurrentIndex( int( mathModeIndex ) );
}


void VoltageDock::setUsed( ChannelID channel, bool used ) {
    if ( channel >= scope->voltage.size() )
        return;
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::setUsed()" << channel << used;
    QSignalBlocker blocker( channelBlocks[ channel ].usedCheckBox );
    channelBlocks[ channel ].usedCheckBox->setChecked( used );
}


void VoltageDock::setInverted( ChannelID channel, bool inverted ) {
    if ( channel >= scope->voltage.size() )
        return;
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::setInverted()" << channel << inverted;
    QSignalBlocker blocker( channelBlocks[ channel ].invertCheckBox );
    channelBlocks[ channel ].invertCheckBox->setChecked( inverted );
}


void VoltageDock::updateGainStrings( double attnValue ) {
    gainStrings.clear();
    for ( auto gainStep : spec->gain ) {
        gainStrings << valueToString( gainStep.Vdiv * attnValue, UNIT_VOLTS, 0 );
    }
}
