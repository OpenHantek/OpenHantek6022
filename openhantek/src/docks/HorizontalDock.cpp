// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "HorizontalDock.h"
#include "dockwindows.h"

#include "scopesettings.h"
#include "sispinbox.h"
#include "utils/printutils.h"

static int row = 0;

template < typename... Args > struct SELECT {
    template < typename C, typename R > static constexpr auto OVERLOAD_OF( R ( C::*pmf )( Args... ) ) -> decltype( pmf ) {
        return pmf;
    }
};

HorizontalDock::HorizontalDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent )
    : QDockWidget( tr( "Horizontal" ), parent ), scope( scope ) {

    if ( scope->verboseLevel > 1 )
        qDebug() << " HorizontalDock::HorizontalDock()";

    // Initialize elements
    this->samplerateLabel = new QLabel( tr( "Samplerate" ) );
    this->samplerateSiSpinBox = new SiSpinBox( UNIT_SAMPLES );
    this->samplerateSiSpinBox->setMinimum( 1 );
    this->samplerateSiSpinBox->setMaximum( 1e8 );
    this->samplerateSiSpinBox->setUnitPostfix( tr( "/s" ) );

    timebaseSteps << 1.0 << 2.0 << 5.0 << 10.0;

    this->timebaseLabel = new QLabel( tr( "Timebase" ) );
    this->timebaseSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    this->timebaseSiSpinBox->setSteps( timebaseSteps );
    this->timebaseSiSpinBox->setMinimum( 1e-9 );
    this->timebaseSiSpinBox->setMaximum( 1e3 );

    this->formatLabel = new QLabel( tr( "Format" ) );
    this->formatComboBox = new QComboBox();
    for ( Dso::GraphFormat format : Dso::GraphFormatEnum )
        this->formatComboBox->addItem( Dso::graphFormatString( format ) );

    this->calfreqLabel = new QLabel( tr( "Calibration out" ) );
    calfreqSteps = spec->calfreqSteps;
    std::reverse( calfreqSteps.begin(), calfreqSteps.end() ); // put highest value on top of the list
    calfreqComboBox = new QComboBox();
    for ( double calfreqStep : calfreqSteps )
        calfreqComboBox->addItem( valueToString( calfreqStep, UNIT_HERTZ, calfreqStep < 10e3 ? 2 : 0 ) );

    this->dockLayout = new QGridLayout();
    this->dockLayout->setColumnMinimumWidth( 0, 64 );
    this->dockLayout->setColumnStretch( 1, 1 );
    this->dockLayout->setSpacing( DOCK_LAYOUT_SPACING );

    row = 0; // allows flexible shift up/down
    this->dockLayout->addWidget( this->timebaseLabel, row, 0 );
    this->dockLayout->addWidget( this->timebaseSiSpinBox, row++, 1 );
    this->dockLayout->addWidget( this->samplerateLabel, row, 0 );
    this->dockLayout->addWidget( this->samplerateSiSpinBox, row++, 1 );
    this->dockLayout->addWidget( this->formatLabel, row, 0 );
    this->dockLayout->addWidget( this->formatComboBox, row++, 1 );
    this->dockLayout->addWidget( this->calfreqLabel, row, 0 );
    this->dockLayout->addWidget( this->calfreqComboBox, row++, 1 );

    this->dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );

    // Load settings into GUI
    this->loadSettings( scope );

    // Connect signals and slots
    connect( this->samplerateSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             &HorizontalDock::samplerateSelected );
    connect( this->timebaseSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             &HorizontalDock::timebaseSelected );
    connect( this->formatComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
             &HorizontalDock::formatSelected );
    connect( calfreqComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ),
             [this]( int index ) { this->calfreqIndexSelected( index ); } );
}


void HorizontalDock::loadSettings( DsoSettingsScope *scope ) {
    // Set values
    this->setSamplerate( scope->horizontal.samplerate );
    this->setTimebase( scope->horizontal.timebase );
    this->setFormat( scope->horizontal.format );
    this->setCalfreq( scope->horizontal.calfreq );
}


/// \brief Don't close the dock, just hide it.
/// \param event The close event that should be handled.
void HorizontalDock::closeEvent( QCloseEvent *event ) {
    this->hide();
    event->accept();
}


double HorizontalDock::setSamplerate( double samplerate ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setSamplerate()" << samplerate;
    samplerateRequest = samplerate;
    QSignalBlocker blocker( timebaseSiSpinBox );
    timebaseSiSpinBox->setMaximum( scope->horizontal.maxTimebase );
    blocker = QSignalBlocker( samplerateSiSpinBox );
    samplerateSiSpinBox->setValue( samplerate );
    return samplerateSiSpinBox->value();
}


double HorizontalDock::setTimebase( double timebase ) {
    QSignalBlocker blocker( timebaseSiSpinBox );
    // timebaseSteps are repeated in each decade
    double decade = pow( 10, floor( log10( timebase ) ) );
    double vNorm = timebase / decade;
    for ( int i = 0; i < timebaseSteps.size() - 1; ++i ) {
        if ( timebaseSteps.at( i ) <= vNorm && vNorm < timebaseSteps.at( i + 1 ) ) {
            timebaseSiSpinBox->setValue( decade * timebaseSteps.at( i ) );
            break;
        }
    }
    calculateSamplerateSteps( timebase );
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setTimebase()" << timebase << "return" << timebaseSiSpinBox->value();
    return timebaseSiSpinBox->value();
}


int HorizontalDock::setFormat( Dso::GraphFormat format ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setFormat()" << format;
    QSignalBlocker blocker( formatComboBox );
    if ( format >= Dso::GraphFormat::TY && format <= Dso::GraphFormat::XY ) {
        formatComboBox->setCurrentIndex( format );
        return format;
    }
    return -1;
}


double HorizontalDock::setCalfreq( double calfreq ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setCalfreq()" << calfreq;
    auto indexIt = std::find( calfreqSteps.begin(), calfreqSteps.end(), calfreq );
    if ( indexIt == calfreqSteps.end() )
        return -1;
    int index = int( std::distance( calfreqSteps.begin(), indexIt ) );
    QSignalBlocker blocker( calfreqComboBox );
    calfreqComboBox->setCurrentIndex( index );
    return calfreq;
}


void HorizontalDock::setSamplerateLimits( double minimum, double maximum ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setSamplerateLimits()" << minimum << maximum;
    QSignalBlocker blocker( samplerateSiSpinBox );
    if ( bool( minimum ) )
        samplerateSiSpinBox->setMinimum( minimum );
    if ( bool( maximum ) )
        samplerateSiSpinBox->setMaximum( maximum );
}


void HorizontalDock::setSamplerateSteps( int mode, const QList< double > steps ) {
    if ( samplerateSteps.size() == steps.size() ) // no action needed
        return;
    if ( scope->verboseLevel > 3 )
        qDebug() << "   HDock::setSamplerateSteps()" << steps;
    else if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setSamplerateSteps()" << steps.first() << "..." << steps.last();
    samplerateSteps = steps;
    // Assume that method is invoked for fixed samplerate devices only
    QSignalBlocker samplerateBlocker( samplerateSiSpinBox );
    samplerateSiSpinBox->setMode( mode );
    samplerateSiSpinBox->setSteps( steps );
    samplerateSiSpinBox->setMinimum( steps.first() );
    samplerateSiSpinBox->setMaximum( steps.last() );
    // Make reasonable adjustments to the timebase spinbox
    QSignalBlocker timebaseBlocker( timebaseSiSpinBox );
    timebaseSiSpinBox->setMinimum( pow( 10, floor( log10( 1.0 / steps.last() ) ) ) );
    calculateSamplerateSteps( timebaseSiSpinBox->value() );
}


/// \brief Called when the samplerate spinbox changes its value.
/// \param samplerate The samplerate in samples/second.
void HorizontalDock::samplerateSelected( double samplerate ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::samplerateSelected()" << samplerate;
    scope->horizontal.samplerate = samplerate;
    emit samplerateChanged( samplerate );
}


/// \brief Called when the timebase spinbox changes its value.
/// \param timebase The timebase in seconds.
void HorizontalDock::timebaseSelected( double timebase ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::timebaseSelected()" << timebase;
    scope->horizontal.timebase = timebase;
    calculateSamplerateSteps( timebase );
    emit timebaseChanged( timebase );
}


void HorizontalDock::calculateSamplerateSteps( double timebase ) {
    int size = samplerateSteps.size();
    if ( size ) {
        // search appropriate min & max sample rate
        double min = samplerateSteps[ 0 ];
        double max = samplerateSteps[ 0 ];
        for ( int id = 0; id < size; ++id ) {
            double sRate = samplerateSteps[ id ];
            if ( scope->verboseLevel > 3 )
                qDebug() << "   sRate, sRate*timebase" << sRate << sRate * timebase;
            // min must be < maxRate
            // find minimal samplerate to get at least this number of samples per div
            if ( id < size - 1 && sRate * timebase <= 10 ) { // 10 samples/div
                min = sRate;
            }
            // max must be > minRate
            // find max samplesrate to get not more then this number of samples per div
            // number should be <= 1000 to get enough samples for two full screens (to ensure triggering)
            if ( id && sRate * timebase <= 1000 ) { // 1000 samples/div
                max = sRate;
            }
        }
        min = qMax( min, qMin( 10e3, max ) ); // not less than 10kS unless max is smaller
        if ( scope->verboseLevel > 2 )
            qDebug() << "  HDock::calculateSamplerateSteps()" << timebase << min << max;
        setSamplerateLimits( min, max );
        // update samplerate if the requested value was limited
        if ( samplerateRequest > samplerateSiSpinBox->value() )
            setSamplerate( samplerateRequest );
    }
}


/// \brief Called when the format combo box changes its value.
/// \param index The index of the combo box item.
void HorizontalDock::formatSelected( int index ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::formatSelected()" << index;
    scope->horizontal.format = Dso::GraphFormat( index );
    emit formatChanged( scope->horizontal.format );
}


/// \brief Called when the calfreq combobox changes its value.
/// \param index The item index.
void HorizontalDock::calfreqIndexSelected( int index ) {
    double calfreq = calfreqSteps[ index ];
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::calfreqIndex Selected()" << index << calfreq;
    scope->horizontal.calfreq = calfreq;
    emit calfreqChanged( calfreq );
}
