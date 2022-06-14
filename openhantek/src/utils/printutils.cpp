// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>

#include <QApplication>
#include <QLocale>
#include <QStringList>

#include "utils/printutils.h"

QString valueToString( double value, Unit unit, int precision ) {
    char format = ( precision < 0 ) ? 'g' : 'f';

    switch ( unit ) {
    case UNIT_VOLTS:
        // Voltage string representation
        if ( bool( fabs( value ) ) ) {
            int logarithm = int( floor( log10( fabs( value ) ) ) );
            if ( fabs( value ) < 1e-3 )
                return QApplication::tr( "%1 µV" ).arg(
                    value * 1e6, 0, format, ( precision <= 0 ) ? precision : qBound( 0, precision - 7 - logarithm, precision ) );
            else if ( fabs( value ) < 1.0 )
                return QApplication::tr( "%1 mV" ).arg( value * 1e3, 0, format,
                                                        ( precision <= 0 ) ? precision : ( precision - 4 - logarithm ) );
            else
                return QApplication::tr( "%1 V" ).arg( value, 0, format,
                                                       ( precision <= 0 ) ? precision : qMax( 0, precision - 1 - logarithm ) );
        } else
            return QApplication::tr( "%1 V" ).arg( 0 );

    case UNIT_VOLTSQUARE:
        // Voltage string representation
        if ( bool( fabs( value ) ) ) {
            int logarithm = int( floor( log10( fabs( value ) ) ) );
            if ( fabs( value ) < 1e-3 )
                return QApplication::tr( "%1 µV²" )
                    .arg( value * 1e6, 0, format,
                          ( precision <= 0 ) ? precision : qBound( 0, precision - 7 - logarithm, precision ) );
            else if ( fabs( value ) < 1.0 )
                return QApplication::tr( "%1 mV²" )
                    .arg( value * 1e3, 0, format, ( precision <= 0 ) ? precision : ( precision - 4 - logarithm ) );
            else
                return QApplication::tr( "%1 V²" ).arg( value, 0, format,
                                                        ( precision <= 0 ) ? precision : qMax( 0, precision - 1 - logarithm ) );
        } else
            return QApplication::tr( "%1 V²" ).arg( 0 );

    case UNIT_WATTS:
        // Watts string representation
        if ( bool( fabs( value ) ) ) {
            int logarithm = int( floor( log10( fabs( value ) ) ) );
            if ( fabs( value ) < 1e-3 )
                return QApplication::tr( "%1 µW" ).arg(
                    value * 1e6, 0, format, ( precision <= 0 ) ? precision : qBound( 0, precision - 7 - logarithm, precision ) );
            else if ( fabs( value ) < 1.0 )
                return QApplication::tr( "%1 mW" ).arg( value * 1e3, 0, format,
                                                        ( precision <= 0 ) ? precision : ( precision - 4 - logarithm ) );
            else
                return QApplication::tr( "%1 W" ).arg( value, 0, format,
                                                       ( precision <= 0 ) ? precision : qMax( 0, precision - 1 - logarithm ) );
        } else
            return QApplication::tr( "%1 W" ).arg( 0 );

    case UNIT_DECIBEL:
        // Power level string representation
        return QApplication::tr( "%1 dB" ).arg(
            value, 0, format,
            ( precision <= 0 ) ? precision : qBound( 0, precision - 1 - int( floor( log10( fabs( value ) ) ) ), precision ) );

    case UNIT_SECONDS:
        // Time string representation
        if ( bool( fabs( value ) ) ) {
            if ( fabs( value ) < 1e-9 )
                return QApplication::tr( "%1 ps" ).arg(
                    value * 1e12, 0, format,
                    ( precision <= 0 ) ? precision
                                       : qBound( 0, precision - 13 - int( floor( log10( fabs( value ) ) ) ), precision ) );
            else if ( fabs( value ) < 1e-6 )
                return QApplication::tr( "%1 ns" ).arg(
                    value * 1e9, 0, format,
                    ( precision <= 0 ) ? precision : ( precision - 10 - int( floor( log10( fabs( value ) ) ) ) ) );
            else if ( fabs( value ) < 1e-3 )
                return QApplication::tr( "%1 µs" ).arg(
                    value * 1e6, 0, format,
                    ( precision <= 0 ) ? precision : ( precision - 7 - int( floor( log10( fabs( value ) ) ) ) ) );
            else if ( fabs( value ) < 1.0 )
                return QApplication::tr( "%1 ms" ).arg(
                    value * 1e3, 0, format,
                    ( precision <= 0 ) ? precision : ( precision - 4 - int( floor( log10( fabs( value ) ) ) ) ) );
            else if ( fabs( value ) < 60 )
                return QApplication::tr( "%1 s" ).arg(
                    value, 0, format, ( precision <= 0 ) ? precision : ( precision - 1 - int( floor( log10( fabs( value ) ) ) ) ) );
            else if ( fabs( value ) < 3600 )
                return QApplication::tr( "%1 min" )
                    .arg( value / 60, 0, format,
                          ( precision <= 0 ) ? precision : ( precision - 1 - int( floor( log10( value / 60 ) ) ) ) );
            else
                return QApplication::tr( "%1 h" ).arg(
                    value / 3600, 0, format,
                    ( precision <= 0 ) ? precision : qMax( 0, precision - 1 - int( floor( log10( value / 3600 ) ) ) ) );
        } else
            return QApplication::tr( "%1 s" ).arg( 0 );

    case UNIT_HERTZ:
        // Frequency string representation
        if ( bool( fabs( value ) ) ) {
            int logarithm = int( floor( log10( fabs( value ) ) ) );
            if ( fabs( value ) < 1e3 )
                return QApplication::tr( "%1 Hz" ).arg(
                    value, 0, format, ( precision <= 0 ) ? precision : qBound( 0, precision - 1 - logarithm, precision ) );
            else if ( fabs( value ) < 1e6 )
                return QApplication::tr( "%1 kHz" )
                    .arg( value * 1e-3, 0, format, ( precision <= 0 ) ? precision : precision + 2 - logarithm );
            else if ( fabs( value ) < 1e9 )
                return QApplication::tr( "%1 MHz" )
                    .arg( value * 1e-6, 0, format, ( precision <= 0 ) ? precision : precision + 5 - logarithm );
            else
                return QApplication::tr( "%1 GHz" )
                    .arg( value * 1e-9, 0, format, ( precision <= 0 ) ? precision : qMax( 0, precision + 8 - logarithm ) );
        } else
            return QApplication::tr( "%1 Hz" ).arg( 0 );

    case UNIT_SAMPLES:
        // Sample count string representation
        if ( bool( fabs( value ) ) ) {
            int logarithm = int( floor( log10( fabs( value ) ) ) );
            if ( fabs( value ) < 1e3 )
                return QApplication::tr( "%1 S" ).arg(
                    value, 0, format, ( precision <= 0 ) ? precision : qBound( 0, precision - 1 - logarithm, precision ) );
            else if ( fabs( value ) < 1e6 )
                return QApplication::tr( "%1 kS" ).arg( value * 1e-3, 0, format,
                                                        ( precision <= 0 ) ? precision : precision + 2 - logarithm );
            else if ( fabs( value ) < 1e9 )
                return QApplication::tr( "%1 MS" ).arg( value * 1e-6, 0, format,
                                                        ( precision <= 0 ) ? precision : precision + 5 - logarithm );
            else
                return QApplication::tr( "%1 GS" ).arg( value * 1e-9, 0, format,
                                                        ( precision <= 0 ) ? precision : qMax( 0, precision + 8 - logarithm ) );
        } else
            return QApplication::tr( "%1 S" ).arg( 0 );

    case UNIT_NONE:
        return QString::number(
            value, format,
            ( precision <= 0 ) ? precision : qBound( 0, precision - 1 - int( floor( log10( fabs( value ) ) ) ), precision ) );

    case UNIT_COUNT:
        return QString::number( int( round( value ) ) );

    default:
        return QString();
    }
}


double stringToValue( const QString &text, Unit unit, bool *ok ) {
    // Check if the text is empty
    int totalSize = text.size();
    if ( !totalSize ) {
        if ( ok )
            *ok = false;
        return 0.0;
    }

    // Split value and unit apart
    int valueSize = 0;
    QLocale locale;
    bool decimalFound = false;
    bool exponentFound = false;
    if ( text[ valueSize ] == locale.negativeSign() )
        ++valueSize;
    for ( ; valueSize < text.size(); ++valueSize ) {
        QChar character = text[ valueSize ];

        if ( character.isDigit() ) {
        } else if ( character == locale.decimalPoint() && decimalFound == false && exponentFound == false ) {
            decimalFound = true;
        } else if ( character == locale.exponential() && exponentFound == false ) {
            exponentFound = true;
            if ( text[ valueSize + 1 ] == locale.negativeSign() )
                ++valueSize;
        } else {
            break;
        }
    }
    QString valueString = text.left( valueSize );
    bool valueOk = false;
    double value = valueString.toDouble( &valueOk );
    if ( !valueOk ) {
        if ( ok )
            *ok = false;
        return value;
    }
    QString unitString = text.right( text.size() - valueSize ).trimmed();

    if ( ok )
        *ok = true;

    switch ( unit ) {
    case UNIT_VOLTS:
    case UNIT_VOLTSQUARE:
    case UNIT_WATTS:
        // Watts string decoding
        if ( unitString.startsWith( "µ" ) ) // my
            return value * 1e-6;
        else if ( unitString.startsWith( 'm' ) )
            return value * 1e-3;
        else if ( unitString.startsWith( 'k' ) )
            return value * 1e3;
        else
            return value;

    case UNIT_DECIBEL:
        // Power level string decoding
        return value;

    case UNIT_SECONDS:
        // Time string decoding
        if ( unitString.startsWith( 'p' ) )
            return value * 1e-12;
        else if ( unitString.startsWith( 'n' ) )
            return value * 1e-9;
        else if ( unitString.startsWith( "µ" ) ) // my
            return value * 1e-6;
        else if ( unitString.startsWith( 'm' ) )
            return value * 1e-3;
        else if ( unitString.startsWith( "min" ) )
            return value * 60;
        else if ( unitString.startsWith( 'h' ) )
            return value * 3600;
        else
            return value;

    case UNIT_HERTZ:
    case UNIT_SAMPLES:
        // Frequency string decoding
        if ( unitString.startsWith( 'k' ) )
            return value * 1e3;
        else if ( unitString.startsWith( 'M' ) )
            return value * 1e6;
        else if ( unitString.startsWith( 'G' ) )
            return value * 1e9;
        else
            return value;

    case UNIT_NONE:
    case UNIT_COUNT:
        return value;

    default:
        if ( ok )
            *ok = false;
        return value;
    }
}


QString hexDump( unsigned char *data, unsigned int length ) {
    QString dumpString;
    for ( unsigned int index = 0; index < length; ++index )
        dumpString.append( QString( "0x%1 " ).arg( data[ index ], 2, 16, QChar( '0' ) ) );
    return dumpString;
}


QString decDump( unsigned char *data, unsigned int length ) {
    QString dumpString;
    for ( unsigned int index = 0; index < length; ++index )
        dumpString.append( QString( "%1 " ).arg( data[ index ] ) );
    return dumpString;
}


QString hexdecDump( unsigned char *data, unsigned int length ) {
    QString dumpString;
    for ( unsigned int index = 0; index < length; ++index )
        dumpString.append( QString( "0x%1 (%2) " ).arg( data[ index ], 2, 16, QChar( '0' ) ).arg( data[ index ] ) );
    return dumpString;
}


unsigned int hexParse( const QString dump, uint8_t *data, unsigned int length ) {
    QString dumpString = dump;
    dumpString.remove( ' ' );
    QString byteString;
    unsigned int index;

    for ( index = 0; index < length; ++index ) {
        byteString = dumpString.mid( int( index ) * 2, 2 );

        // Check if we reached the end of the string
        if ( byteString.isNull() )
            break;

        // Check for parsing errors
        bool ok;
        uint8_t byte = uint8_t( byteString.toUShort( &ok, 16 ) );
        if ( !ok )
            break;

        data[ index ] = byte;
    }

    return index;
}
