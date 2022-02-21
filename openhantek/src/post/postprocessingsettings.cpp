// SPDX-License-Identifier: GPL-2.0+

#include "postprocessingsettings.h"

#include <QCoreApplication>
#include <QString>

namespace Dso {

// both Enum definitions must match the "extern" declarations in "postprocessingsettings.h"
Enum< Dso::MathMode, Dso::MathMode::ADD_CH1_CH2, Dso::MathMode::TRIG_CH2 > MathModeEnum;
Enum< Dso::WindowFunction, Dso::WindowFunction::RECTANGULAR, Dso::WindowFunction::FLATTOP > WindowFunctionEnum;


Unit mathModeUnit( MathMode mode ) {
    if ( mode == MathMode::MUL_CH1_CH2 || mode == MathMode::SQ_CH1 || mode == MathMode::SQ_CH2 )
        return UNIT_VOLTSQUARE;
    else
        return UNIT_VOLTS;
}


/// \brief Return string representation of the given math mode.
/// \param mode The ::MathMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString mathModeString( MathMode mode ) {
    switch ( mode ) {
    case MathMode::ADD_CH1_CH2:
        return QCoreApplication::tr( "CH1+CH2" );
    case MathMode::SUB_CH2_FROM_CH1:
        return QCoreApplication::tr( "CH1-CH2" );
    case MathMode::SUB_CH1_FROM_CH2:
        return QCoreApplication::tr( "CH2-CH1" );
    case MathMode::MUL_CH1_CH2:
        return QCoreApplication::tr( "CH1*CH2" );
    case MathMode::SQ_CH1:
        return QCoreApplication::tr( "CH1 ^2" );
    case MathMode::SQ_CH2:
        return QCoreApplication::tr( "CH2 ^2" );
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
