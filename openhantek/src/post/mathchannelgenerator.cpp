// SPDX-License-Identifier: GPL-2.0+

#include <QDebug>

#include "enums.h"
#include "mathchannelgenerator.h"
#include "post/postprocessingsettings.h"
#include "scopesettings.h"

MathChannelGenerator::MathChannelGenerator( const DsoSettingsScope *scope, unsigned physicalChannels )
    : mathChannel( physicalChannels ), scope( scope ) {
    if ( scope->verboseLevel > 1 )
        qDebug() << " MathChannelGenerator::MathChannelGenerator()";
}


MathChannelGenerator::~MathChannelGenerator() {
    if ( scope->verboseLevel > 1 )
        qDebug() << " MathChannelGenerator::~MathChannelGenerator()";
}


void MathChannelGenerator::process( PPresult *result ) {
    // Math channel enabled?
    if ( !scope->voltage[ mathChannel ].used && !scope->spectrum[ mathChannel ].used )
        return;

    if ( scope->verboseLevel > 4 )
        qDebug() << "    MathChannelGenerator::process()" << result->tag;

    DataChannel *const channelData = result->modifiableData( mathChannel );
    std::vector< double > &resultData = channelData->voltage.sample;

    const double sign = scope->voltage[ mathChannel ].inverted ? -1.0 : 1.0;

    if ( Dso::getMathMode( scope->voltage[ mathChannel ] ) < Dso::MathMode::AC_CH1 ) { // binary operations
        if ( result->data( 0 )->voltage.sample.empty() || result->data( 1 )->voltage.sample.empty() )
            return;
        // Resize the sample vector
        resultData.resize( std::min( result->data( 0 )->voltage.sample.size(), result->data( 1 )->voltage.sample.size() ) );
        // Set sampling interval
        channelData->voltage.interval = result->data( 0 )->voltage.interval;
        // Calculate values and write them into the sample buffer
        std::vector< double >::const_iterator ch1Iterator = result->data( 0 )->voltage.sample.begin();
        std::vector< double >::const_iterator ch2Iterator = result->data( 1 )->voltage.sample.begin();
        double ( *calculate )( double, double );

        switch ( Dso::getMathMode( scope->voltage[ mathChannel ] ) ) {
        case Dso::MathMode::ADD_CH1_CH2:
            calculate = []( double val1, double val2 ) -> double { return val1 + val2; };
            break;
        case Dso::MathMode::SUB_CH2_FROM_CH1:
            calculate = []( double val1, double val2 ) -> double { return val1 - val2; };
            break;
        case Dso::MathMode::SUB_CH1_FROM_CH2:
            calculate = []( double val1, double val2 ) -> double { return val2 - val1; };
            break;
        case Dso::MathMode::MUL_CH1_CH2:
            // multiply e.g. voltage and current (measured with a 1 ohm shunt) to get momentary power
            calculate = []( double val1, double val2 ) -> double { return val1 * val2; };
            break;
        default:
            calculate = []( double, double ) -> double { return 0.0; };
            break;
        }
        for ( auto it = resultData.begin(), end = resultData.end(); it != end; ++it ) {
            *it = sign * calculate( *ch1Iterator++, *ch2Iterator++ );
        }
    } else { // unary operators (calculate "AC coupling" or DC value)
        unsigned src = 0;
        if ( Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::AC_CH1 or
             Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::DC_CH1 )
            src = 0;
        else if ( Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::AC_CH2 or
                  Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::DC_CH2 )
            src = 1;

        // Resize the sample vector
        resultData.resize( result->data( src )->voltage.sample.size() );
        // Set sampling interval
        channelData->voltage.interval = result->data( src )->voltage.interval;

        // calculate DC component of channel...
        double average = 0;
        for ( auto srcIt = result->data( src )->voltage.sample.begin(), srcEnd = result->data( src )->voltage.sample.end();
              srcIt != srcEnd; ++srcIt ) {
            average += *srcIt;
        }
        average /= double( result->data( src )->voltage.sample.size() );

        auto srcIt = result->data( src )->voltage.sample.begin();
        if ( Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::AC_CH1 or
             Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::AC_CH2 )
            // ... and remove DC component to get AC
            for ( auto dstIt = resultData.begin(), dstEnd = resultData.end(); dstIt != dstEnd; ++srcIt, ++dstIt )
                *dstIt = sign * ( *srcIt - average );
        else if ( Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::DC_CH1 or
                  Dso::getMathMode( scope->voltage[ mathChannel ] ) == Dso::MathMode::DC_CH2 )
            // ... and show DC component
            for ( auto dstIt = resultData.begin(), dstEnd = resultData.end(); dstIt != dstEnd; ++srcIt, ++dstIt )
                *dstIt = sign * average;
    }
}
