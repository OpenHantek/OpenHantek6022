// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <cerrno>

#include <QString>
#include <QTime>

//////////////////////////////////////////////////////////////////////////////
/// \enum Unit utils/printutils.h
/// \brief The various units supported by valueToString.
enum Unit { UNIT_NONE, UNIT_VOLTS, UNIT_DECIBEL, UNIT_SECONDS, UNIT_HERTZ, UNIT_SAMPLES, UNIT_COUNT, UNIT_WATTS, UNIT_VOLTSQUARE };

/// \brief Converts double to string containing value and (prefix+)unit
/// (Counterpart to stringToValue).
/// \param value The value in prefixless units.
/// \param unit The unit for the value.
/// \param precision Significant digits, 0 for integer, -1 for auto.
/// \return String with the value and unit.
QString valueToString( double value, Unit unit, int precision = -1 );

/// \brief Converts string containing value and (prefix+)unit to double
/// (Counterpart to valueToString).
/// \param text The text containing the value and its unit.
/// \param unit The base unit of the value.
/// \param ok Pointer to a success-flag, true on success, false on error.
/// \return Decoded value.
double stringToValue( const QString &text, Unit unit, bool *ok = nullptr );

/// \brief Returns the hex or decimal dump for the given data.
/// \param data Pointer to the data bytes that should be dumped.
/// \param length The length of the data array in bytes.
/// \return String with the dump of the data.
QString hexDump( unsigned char *data, unsigned int length );
QString decDump( unsigned char *data, unsigned int length );
QString hexdecDump( unsigned char *data, unsigned int length );

/// \brief Returns the hex dump for the given data.
/// \param dump The string with the hex dump of the data.
/// \param data Pointer to the address where the data bytes should be saved.
/// \param length The maximum length of the data array in bytes.
/// \return The length of the saved data.
unsigned int hexParse( const QString dump, unsigned char *data, unsigned int length );

/// \brief Print debug information with timestamp.
/// \param text Text that will be output via qDebug.
#ifdef TIMESTAMPDEBUG
inline void timestampDebug( const QString &text ) {
    qDebug( "%s: %s", QTime::currentTime().toString( "hh:mm:ss.zzz" ).toLatin1().constData(), text.toLatin1().constData() );
}
#else
#define timestampDebug( ARG )
#endif
