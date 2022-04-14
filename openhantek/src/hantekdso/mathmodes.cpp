#include "mathmodes.h"
#include <QCoreApplication>

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
