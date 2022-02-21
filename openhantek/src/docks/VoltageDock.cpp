// SPDX-License-Identifier: GPL-2.0+

#include <QCloseEvent>
#include <QDebug>
#include <QDockWidget>
#include <QSignalBlocker>

#include <cmath>

#include "VoltageDock.h"
#include "dockwindows.h"

#include "dsosettings.h"
#include "sispinbox.h"
#include "utils/printutils.h"


template < typename... Args > struct SELECT {
    template < typename C, typename R > static constexpr auto OVERLOAD_OF( R ( C::*pmf )( Args... ) ) -> decltype( pmf ) {
        return pmf;
    }
};


VoltageDock::VoltageDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent )
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

    for ( double gainStep : scope->gainSteps ) {
        gainStrings << valueToString( gainStep, UNIT_VOLTS, 0 );
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

        if ( channel < spec->channels )
            b.usedCheckBox = new QCheckBox( tr( "CH&%1" ).arg( channel + 1 ) ); // define shortcut <ALT>1 / <ALT>2
        else
            b.usedCheckBox = new QCheckBox( tr( "MA&TH" ) );
        b.miscComboBox = new QComboBox();
        b.gainComboBox = new QComboBox();
        b.invertCheckBox = new QCheckBox( tr( "Invert" ) );
        b.attnSpinBox = new QSpinBox();
        b.attnSpinBox->setMinimum( ATTENUATION_MIN );
        b.attnSpinBox->setMaximum( ATTENUATION_MAX );
        b.attnSpinBox->setPrefix( tr( "x" ) );

        channelBlocks.push_back( std::move( b ) );

        if ( channel < spec->channels )
            b.miscComboBox->addItems( couplingStrings );
        else
            b.miscComboBox->addItems( modeStrings );

        b.gainComboBox->addItems( gainStrings );

        dockLayout->setColumnStretch( 1, 1 ); // stretch ComboBox in 2nd (middle) column 1x
        dockLayout->setColumnStretch( 2, 2 ); // stretch ComboBox in 3rd (last) column 2x
        dockLayout->addWidget( b.usedCheckBox, row, 0 );
        dockLayout->addWidget( b.gainComboBox, row++, 1, 1, 2 ); // fill 1 row, 2 col
        dockLayout->addWidget( b.invertCheckBox, row, 0 );
        dockLayout->addWidget( b.attnSpinBox, row, 1, 1, 1 );    // fill 1 row, 2 col
        dockLayout->addWidget( b.miscComboBox, row++, 2, 1, 1 ); // fill 1 row, 2 col

        // draw divider line
        if ( channel < spec->channels ) {
            QFrame *divider = new QFrame();
            divider->setLineWidth( 1 );
            divider->setFrameShape( QFrame::HLine );
            QPalette palette = QPalette();
            palette.setColor( QPalette::WindowText, QColor( 128, 128, 128 ) );
            divider->setPalette( palette ); // reduce the contrast of the divider
            dockLayout->addWidget( divider, row++, 0, 1, 3 );
        }

        connect( b.gainComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ),
                 [ this, channel ]( unsigned index ) {
                     this->scope->voltage[ channel ].gainStepIndex = index;
                     emit gainChanged( channel, this->scope->gain( channel ) );
                 } );
        connect( b.attnSpinBox, SELECT< int >::OVERLOAD_OF( &QSpinBox::valueChanged ), [ this, channel ]( unsigned attnValue ) {
            this->scope->voltage[ channel ].probeAttn = attnValue;
            setAttn( channel, attnValue );
            emit probeAttnChanged( channel, attnValue ); // make sure to set the probe first, since this will influence the gain
            emit gainChanged( channel, this->scope->gain( channel ) );
        } );
        connect( b.invertCheckBox, &QAbstractButton::toggled, [ this, channel ]( bool checked ) {
            this->scope->voltage[ channel ].inverted = checked;
            emit invertedChanged( channel, checked );
        } );
        connect( b.miscComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ),
                 [ this, channel, spec, scope ]( unsigned index ) {
                     this->scope->voltage[ channel ].couplingOrMathIndex = index;
                     if ( channel < spec->channels ) {
                         // setCoupling(channel, (unsigned)index);
                         emit couplingChanged( channel, scope->coupling( channel, spec ) );
                     } else {                                                           // MATH function changed
                         setAttn( channel, this->scope->voltage[ channel ].probeAttn ); // update unit
                         emit modeChanged( Dso::getMathMode( this->scope->voltage[ channel ] ) );
                     }
                 } );
        connect( b.usedCheckBox, &QAbstractButton::toggled, [ this, channel ]( bool checked ) {
            this->scope->voltage[ channel ].used = checked;
            emit usedChanged( channel, checked );
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
    QSignalBlocker blocker( channelBlocks[ channel ].miscComboBox );
    channelBlocks[ channel ].miscComboBox->setCurrentIndex( int( couplingIndex ) );
}


void VoltageDock::setGain( ChannelID channel, unsigned gainStepIndex ) {
    if ( channel >= scope->voltage.size() )
        return;
    if ( gainStepIndex >= scope->gainSteps.size() )
        return;
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::setGain()" << channel << gainStrings[ int( gainStepIndex ) ];
    QSignalBlocker blocker( channelBlocks[ channel ].gainComboBox );
    channelBlocks[ channel ].gainComboBox->setCurrentIndex( int( gainStepIndex ) );
}


void VoltageDock::setAttn( ChannelID channel, double attnValue ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  VDock::setAttn()" << channel << attnValue;
    if ( channel >= scope->voltage.size() )
        return;
    QSignalBlocker blocker( channelBlocks[ channel ].gainComboBox );
    int index = channelBlocks[ channel ].gainComboBox->currentIndex();
    gainStrings.clear();
    // change unit to V² for the multiplying math functions
    if ( channel >= spec->channels && // MATH channel
         ( ( scope->voltage[ spec->channels ].couplingOrMathIndex == unsigned( Dso::MathMode::MUL_CH1_CH2 ) ) ||
           ( scope->voltage[ spec->channels ].couplingOrMathIndex == unsigned( Dso::MathMode::SQ_CH1 ) ) ||
           ( scope->voltage[ spec->channels ].couplingOrMathIndex == unsigned( Dso::MathMode::SQ_CH2 ) ) ) ) {
        for ( double gainStep : scope->gainSteps ) {
            gainStrings << valueToString( gainStep * attnValue, UNIT_VOLTSQUARE, -1 ); // auto format V²
        }
    } else {
        for ( double gainStep : scope->gainSteps ) {
            gainStrings << valueToString( gainStep * attnValue, UNIT_VOLTS, -1 ); // auto format V
        }
    }
    channelBlocks[ channel ].gainComboBox->clear();
    channelBlocks[ channel ].gainComboBox->addItems( gainStrings );
    channelBlocks[ channel ].gainComboBox->setCurrentIndex( index );
    scope->voltage[ channel ].probeAttn = attnValue;
    channelBlocks[ channel ].attnSpinBox->setValue( int( attnValue ) );
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
