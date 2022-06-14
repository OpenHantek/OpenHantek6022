// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDebug>

#include "hantekdsocontrol.h"
#include "mathchannel.h"
#include "mathmodes.h"
#include <cmath>


MathChannel::MathChannel( const DsoSettingsScope *scope ) : scope( scope ) {
    if ( scope->verboseLevel > 1 )
        qDebug() << " MathChannel::MathChannel()";
}


void MathChannel::calculate( DSOsamples &result ) {
    const size_t CH1 = 0;
    const size_t CH2 = 1;
    const size_t MATH = 2;
    const double sign = scope->voltage[ MATH ].inverted ? -1.0 : 1.0;
    QWriteLocker resultLocker( &result.lock );
    std::vector< double > &mathChannel = result.data[ MATH ];
    const size_t resultSamples = result.data[ CH1 ].size();
    const Dso::MathMode mathMode = Dso::getMathMode( scope->voltage[ MATH ] );
    mathChannel.resize( resultSamples );
    if ( mathMode <= Dso::LastBinaryMathMode ) { // binary operations
        if ( result.data[ CH1 ].empty() || result.data[ CH2 ].empty() )
            return;

        // Calculate values and write them into the sample buffer
        std::vector< double >::const_iterator ch1Iterator = result.data[ CH1 ].begin();
        std::vector< double >::const_iterator ch2Iterator = result.data[ CH2 ].begin();

        if ( result.clipped & 0x03 ) // at least one channel has clipped
            result.clipped |= 0x04;  // .. the math channel is not reliable
        else
            result.clipped &= ~0x04; // clear clipping

        switch ( mathMode ) {
        case Dso::MathMode::ADD_CH1_CH2:
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator ) {
                *it = sign * ( *ch1Iterator + *ch2Iterator );
            }
            break;
        case Dso::MathMode::SUB_CH2_FROM_CH1:
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator ) {
                *it = sign * ( *ch1Iterator - *ch2Iterator );
            }
            break;
        case Dso::MathMode::SUB_CH1_FROM_CH2:
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator ) {
                *it = sign * ( *ch2Iterator - *ch1Iterator );
            }
            break;
        case Dso::MathMode::MUL_CH1_CH2:
            // multiply e.g. voltage and current (measured with a 1 ohm shunt) to get momentary power
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator ) {
                *it = sign * ( *ch1Iterator * *ch2Iterator );
            }
            break;
        case Dso::MathMode::AND_CH1_CH2:
            // logic values: above / below trigger level
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator )
                *it = ( ( *ch1Iterator >= scope->voltage[ CH1 ].trigger ) && ( *ch2Iterator >= scope->voltage[ CH2 ].trigger )
                            ? 1.0
                            : 0.0 );
            break;
        case Dso::MathMode::AND_NOT_CH1_CH2:
            // logic values: above / below trigger level
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator )
                *it = ( !( *ch1Iterator >= scope->voltage[ CH1 ].trigger ) && ( *ch2Iterator >= scope->voltage[ CH2 ].trigger )
                            ? 1.0
                            : 0.0 );
            break;
        case Dso::MathMode::AND_CH1_NOT_CH2:
            // logic values: above / below trigger level
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator )
                *it = ( ( *ch1Iterator >= scope->voltage[ CH1 ].trigger ) && !( *ch2Iterator >= scope->voltage[ CH2 ].trigger )
                            ? 1.0
                            : 0.0 );
            break;
        case Dso::MathMode::AND_NOT_CH1_NOT_CH2:
            // logic values: above / below trigger level
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator )
                *it = ( !( *ch1Iterator >= scope->voltage[ CH1 ].trigger ) && !( *ch2Iterator >= scope->voltage[ CH2 ].trigger )
                            ? 1.0
                            : 0.0 );
            break;
        case Dso::MathMode::EQU_CH1_CH2:
            // logic values: above / below trigger level
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator )
                *it = ( ( *ch1Iterator >= scope->voltage[ CH1 ].trigger ) == ( *ch2Iterator >= scope->voltage[ CH2 ].trigger )
                            ? 1.0
                            : 0.0 );
            break;
        default:
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it )
                *it = 0.0;
            break;
        }
    } else {           // unary operators (calculate square, AC, DC, abs, sign, ...)
        unsigned src = // alternating 0 and 1 for the unary math functions
            ( unsigned( mathMode ) - unsigned( Dso::LastBinaryMathMode ) - 1 ) & 0x01;

        if ( result.data[ src ].empty() )
            return;

        if ( result.clipped & 0x01 << src ) // the input channel has clipped
            result.clipped |= 0x04;         // .. the math channel is not reliable
        else
            result.clipped &= ~0x04; // clear clipping

        if ( mathMode == Dso::MathMode::SQ_CH1 || mathMode == Dso::MathMode::SQ_CH2 ) {
            auto srcIt = result.data[ src ].begin();
            for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt )
                *dstIt = sign * ( *srcIt * *srcIt );
        } else {
            // calculate DC component of channel that's needed for some of the math functions...
            double average = 0;
            for ( auto srcIt = result.data[ src ].begin(), srcEnd = result.data[ src ].end(); srcIt != srcEnd; ++srcIt ) {
                average += *srcIt;
            }
            average /= double( result.data[ src ].size() );

            // also needed for all math functions
            auto srcIt = result.data[ src ].begin();

            switch ( mathMode ) {
            case Dso::MathMode::LP10_CH1:
            case Dso::MathMode::LP10_CH2:
            case Dso::MathMode::LP100_CH1:
            case Dso::MathMode::LP100_CH2: {
                // zero-phase (non-causal) bidirectional low-pass filtering
                // Steven W. Smith: The Scientist and Engineer's Guide to Digital Signal Processing, ch. 19
                // set IIR filter coefficients a0 and b1 for tau = 10 or 100 samples (10000 samples on screen)
                // for less on-screen-samples adapt the values according equation 19-4
                double normalScreenSamples = double( result.data[ MATH ].size() ) / 2; // normally 10000
                double a0, b1;
                if ( mathMode == Dso::MathMode::LP10_CH1 || mathMode == Dso::MathMode::LP10_CH2 )
                    b1 = exp( -normalScreenSamples / scope->horizontal.dotsOnScreen / 10 ); // eq. 19-4
                else
                    b1 = exp( -normalScreenSamples / scope->horizontal.dotsOnScreen / 100 ); // eq. 19-4
                a0 = 1 - b1;
                // filter from left to right
                double y = average;
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt = y / 2; // 50% contribution
                    y = a0 * *srcIt + b1 * y;
                }
                // add the filter result from right to left
                auto srcIt = result.data[ src ].rbegin();
                y = average;
                for ( auto dstIt = mathChannel.rbegin(), dstEnd = mathChannel.rend(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt += y / 2; // the 2nd 50%
                    y = a0 * *srcIt + b1 * y;
                }
            } break;
            case Dso::MathMode::AC_CH1:
            case Dso::MathMode::AC_CH2:
                // remove DC component to get AC
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt )
                    *dstIt = sign * ( *srcIt - average );
                break;
            case Dso::MathMode::DC_CH1:
            case Dso::MathMode::DC_CH2:
                // and show DC component
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt = sign * average;
                }
                break;
            case Dso::MathMode::ABS_CH1:
            case Dso::MathMode::ABS_CH2:
                // absolute value of signal
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt = sign * ( *srcIt < 0 ? -*srcIt : *srcIt );
                }
                break;
            case Dso::MathMode::SIGN_CH1:
            case Dso::MathMode::SIGN_CH2:
                // positive: 1, zero: 0, negative -1
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt = sign * ( *srcIt < 0 ? -1 : 1 );
                }
                break;
            case Dso::MathMode::SIGN_AC_CH1:
            case Dso::MathMode::SIGN_AC_CH2:
                // same for AC part of signal
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt = sign * ( *srcIt < average ? -1 : 1 );
                }
                break;
            case Dso::MathMode::TRIG_CH1:
            case Dso::MathMode::TRIG_CH2:
                // above / below trigger level
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt = ( *srcIt < scope->voltage[ src ].trigger ? 0 : 1 );
                }
                break;
            default:
                break;
            }
        }
    }
    result.mathVoltageUnit = mathModeUnit( mathMode );
}
