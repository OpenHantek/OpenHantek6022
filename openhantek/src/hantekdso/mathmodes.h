// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "utils/enumclass.h"
#include "utils/printutils.h"

#include <QMetaType>

namespace Dso {

/// \enum MathMode
/// \brief The different math modes for the math-channel.
enum class MathMode : unsigned {
    // binary arithmetical functions
    ADD_CH1_CH2,
    SUB_CH2_FROM_CH1,
    SUB_CH1_FROM_CH2,
    MUL_CH1_CH2,
    // binary logical functions
    AND_CH1_CH2,
    AND_NOT_CH1_NOT_CH2,
    AND_NOT_CH1_CH2,
    AND_CH1_NOT_CH2,
    EQU_CH1_CH2,
    // unary arithmetical functions
    LP10_CH1,
    LP10_CH2,
    LP100_CH1,
    LP100_CH2,
    SQ_CH1,
    SQ_CH2,
    AC_CH1,
    AC_CH2,
    DC_CH1,
    DC_CH2,
    ABS_CH1,
    ABS_CH2,
    SIGN_CH1,
    SIGN_CH2,
    SIGN_AC_CH1,
    SIGN_AC_CH2,
    // unary logical functions
    TRIG_CH1,
    TRIG_CH2
};
// this "extern" declaration must match the Enum definition in "mathchannel.cpp"
extern Enum< Dso::MathMode, Dso::MathMode::ADD_CH1_CH2, Dso::MathMode::TRIG_CH2 > MathModeEnum;

const auto LastBinaryMathMode = MathMode::EQU_CH1_CH2;
const auto LastMathMode = MathMode::TRIG_CH2;

Unit mathModeUnit( MathMode mode );

QString mathModeString( MathMode mode );

unsigned mathChannelsUsed( MathMode mode );

template < class T > inline MathMode getMathMode( T &t ) { return MathMode( t.couplingOrMathIndex ); }


} // namespace Dso

Q_DECLARE_METATYPE( Dso::MathMode )
