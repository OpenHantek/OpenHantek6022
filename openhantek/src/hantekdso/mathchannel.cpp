// SPDX-License-Identifier: GPL-2.0+

#include <QCoreApplication>

#include "hantekdsocontrol.h"
#include "mathmodes.h"

void HantekDsoControl::createMathChannel() {
    const size_t CH1 = 0;
    const size_t CH2 = 1;
    const size_t MATH = 2;
    const double sign = controlsettings.voltage[ MATH ].inverted ? -1.0 : 1.0;
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
                *it = ( ( *ch1Iterator >= this->scope->voltage[ CH1 ].trigger ) &&
                                !( *ch2Iterator >= this->scope->voltage[ CH2 ].trigger )
                            ? 1.0
                            : 0.0 );
            break;
        case Dso::MathMode::AND_NOT_CH1_NOT_CH2:
            // logic values: above / below trigger level
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator )
                *it = ( !( *ch1Iterator >= this->scope->voltage[ CH1 ].trigger ) &&
                                !( *ch2Iterator >= this->scope->voltage[ CH2 ].trigger )
                            ? 1.0
                            : 0.0 );
            break;
        case Dso::MathMode::EQU_CH1_CH2:
            // logic values: above / below trigger level
            for ( auto it = mathChannel.begin(), end = mathChannel.end(); it != end; ++it, ++ch1Iterator, ++ch2Iterator )
                *it = ( ( *ch1Iterator >= this->scope->voltage[ CH1 ].trigger ) ==
                                ( *ch2Iterator >= this->scope->voltage[ CH2 ].trigger )
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
            // Low pass filter, tau = 10 or 100 samples
            case Dso::MathMode::LP10_CH1:
            case Dso::MathMode::LP10_CH2:
            case Dso::MathMode::LP100_CH1:
            case Dso::MathMode::LP100_CH2: {
                // do a phase correct but non-causal lp filtering
                // set IIR filter coefficients
                double a;
                if ( mathMode == Dso::MathMode::LP10_CH1 || mathMode == Dso::MathMode::LP10_CH2 )
                    a = 0.1;
                else
                    a = 0.01;
                const double b = 1 - a;

                // filter from left to right
                double y = average;
                for ( auto dstIt = mathChannel.begin(), dstEnd = mathChannel.end(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt = y / 2;
                    y = a * *srcIt + b * y;
                }
                // add the filter result from right to left
                y = average;
                auto srcIt = result.data[ src ].rbegin();
                for ( auto dstIt = mathChannel.rbegin(), dstEnd = mathChannel.rend(); dstIt != dstEnd; ++srcIt, ++dstIt ) {
                    *dstIt += y / 2;
                    y = a * *srcIt + b * y;
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


namespace Dso {

// Enum definition must match the "extern" declarations in "mathmodes.h"
Enum< Dso::MathMode, Dso::MathMode::ADD_CH1_CH2, Dso::MathMode::TRIG_CH2 > MathModeEnum;

Unit mathModeUnit( MathMode mode ) {
    if ( mode == MathMode::MUL_CH1_CH2 || mode == MathMode::SQ_CH1 || mode == MathMode::SQ_CH2 )
        return UNIT_VOLTSQUARE;
    else if ( mode == MathMode::AND_CH1_CH2 || mode == MathMode::AND_NOT_CH1_CH2 || mode == MathMode::AND_CH1_NOT_CH2 ||
              mode == MathMode::AND_NOT_CH1_NOT_CH2 || mode == MathMode::EQU_CH1_CH2 || mode == MathMode::SIGN_AC_CH1 ||
              mode == MathMode::SIGN_AC_CH2 || mode == MathMode::SIGN_CH1 || mode == MathMode::SIGN_CH2 ||
              mode == MathMode::TRIG_CH2 || mode == MathMode::TRIG_CH1 || mode == MathMode::TRIG_CH2 )
        return UNIT_NONE; // logic values 0 or 1
    else
        return UNIT_VOLTS;
}

unsigned mathChannelsUsed( MathMode mode ) {
    if ( mode <= LastBinaryMathMode ) // use both channels
        return 3;                     // 0b11
    else                              // use alternating CH1 (0b01), CH2 (0b10), CH1, ...
        return ( ( unsigned( mode ) - unsigned( LastBinaryMathMode ) - 1 ) & 1 ) + 1;
}

/// \brief Return string representation of the given math mode.
/// \param mode The ::MathMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString mathModeString( MathMode mode ) {
    switch ( mode ) {
    case MathMode::ADD_CH1_CH2:
        return QCoreApplication::tr( "CH1 + CH2" );
    case MathMode::SUB_CH2_FROM_CH1:
        return QCoreApplication::tr( "CH1 - CH2" );
    case MathMode::SUB_CH1_FROM_CH2:
        return QCoreApplication::tr( "CH2 - CH1" );
    case MathMode::MUL_CH1_CH2:
        return QCoreApplication::tr( "CH1 * CH2" );
    case MathMode::AND_CH1_CH2:
        return QCoreApplication::tr( "CH1 & CH2" );
    case MathMode::AND_NOT_CH1_NOT_CH2:
        return QCoreApplication::tr( "/CH1 & /CH2" );
    case MathMode::AND_NOT_CH1_CH2:
        return QCoreApplication::tr( "/CH1 & CH2" );
    case MathMode::AND_CH1_NOT_CH2:
        return QCoreApplication::tr( "CH1 & /CH2" );
    case MathMode::EQU_CH1_CH2:
        return QCoreApplication::tr( "CH1 == CH2" );
    case MathMode::LP10_CH1:
        return QCoreApplication::tr( "CH1 LP10" );
    case MathMode::LP10_CH2:
        return QCoreApplication::tr( "CH2 LP10" );
    case MathMode::LP100_CH1:
        return QCoreApplication::tr( "CH1 LP100" );
    case MathMode::LP100_CH2:
        return QCoreApplication::tr( "CH2 LP100" );
    case MathMode::SQ_CH1:
        return QCoreApplication::tr( "CH1²" );
    case MathMode::SQ_CH2:
        return QCoreApplication::tr( "CH2²" );
    case MathMode::AC_CH1:
        return QCoreApplication::tr( "CH1 AC" );
    case MathMode::AC_CH2:
        return QCoreApplication::tr( "CH2 AC" );
    case MathMode::DC_CH1:
        return QCoreApplication::tr( "CH1 DC" );
    case MathMode::DC_CH2:
        return QCoreApplication::tr( "CH2 DC" );
    case MathMode::ABS_CH1:
        return QCoreApplication::tr( "CH1 Abs" );
    case MathMode::ABS_CH2:
        return QCoreApplication::tr( "CH2 Abs" );
    case MathMode::SIGN_CH1:
        return QCoreApplication::tr( "CH1 Sign" );
    case MathMode::SIGN_CH2:
        return QCoreApplication::tr( "CH2 Sign" );
    case MathMode::SIGN_AC_CH1:
        return QCoreApplication::tr( "CH1 AC Sign" );
    case MathMode::SIGN_AC_CH2:
        return QCoreApplication::tr( "CH2 AC Sign" );
    case MathMode::TRIG_CH1:
        return QCoreApplication::tr( "CH1 Trigger" );
    case MathMode::TRIG_CH2:
        return QCoreApplication::tr( "CH2 Trigger" );
    }
    return QString();
}

} // namespace Dso
