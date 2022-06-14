// SPDX-License-Identifier: GPL-2.0-or-later

#include "enums.h"
#include <QCoreApplication>

namespace Dso {
Enum< Dso::TriggerMode, Dso::TriggerMode::AUTO, Dso::TriggerMode::ROLL > TriggerModeEnum;
Enum< Dso::Slope, Dso::Slope::Positive, Dso::Slope::Both > SlopeEnum;
Enum< Dso::GraphFormat, Dso::GraphFormat::TY, Dso::GraphFormat::XY > GraphFormatEnum;

/// \brief Return string representation of the given graph format.
/// \param format The ::GraphFormat that should be returned as string.
/// \return The string that should be used in labels etc.
QString graphFormatString( GraphFormat format ) {
    switch ( format ) {
    case GraphFormat::TY:
        return QCoreApplication::tr( "T - Y" );
    case GraphFormat::XY:
        return QCoreApplication::tr( "X - Y" );
    }
    return QString();
}

/// \brief Return string representation of the given channel coupling.
/// \param coupling The ::Coupling that should be returned as string.
/// \return The string that should be used in labels etc.
QString couplingString( Coupling coupling ) {
    switch ( coupling ) {
    case Coupling::AC:
        return QCoreApplication::tr( "AC" );
    case Coupling::DC:
        return QCoreApplication::tr( "DC" );
    case Coupling::GND:
        return QCoreApplication::tr( "GND" );
    }
    return QString();
}


/// \brief Return string representation of the given trigger mode.
/// \param mode The ::TriggerMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString triggerModeString( TriggerMode mode ) {
    switch ( mode ) {
    case TriggerMode::AUTO:
        return QCoreApplication::tr( "Auto" );
    case TriggerMode::NORMAL:
        return QCoreApplication::tr( "Normal" );
    case TriggerMode::SINGLE:
        return QCoreApplication::tr( "Single" );
    case TriggerMode::ROLL:
        return QCoreApplication::tr( "Roll" );
    }
    return QString();
}

/// \brief Return string representation of the given trigger slope.
/// \param slope The ::Slope that should be returned as string.
/// \return The string that should be used in labels etc.
QString slopeString( Slope slope ) {
    switch ( slope ) {
#if defined Q_OS_WIN // avoid unicode mismatch
    case Slope::Positive:
        return QString::fromUtf8( "/" );
    case Slope::Negative:
        return QString::fromUtf8( "\\" );
    case Slope::Both:
        return QString::fromUtf8( "X" );
#else
    case Slope::Positive:
        return QString::fromUtf8( "\u2197" ); // "↗"
    case Slope::Negative:
        return QString::fromUtf8( "\u2198" ); // "↘"
    case Slope::Both:
        return QString::fromUtf8( "\u2928" ); // "⤨"
#endif
    }
    return QString();
}

} // namespace Dso
