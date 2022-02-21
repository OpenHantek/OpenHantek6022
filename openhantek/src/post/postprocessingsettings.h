// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "utils/enumclass.h"
#include "utils/printutils.h"

#include <QMetaType>

namespace Dso {

/// \enum MathMode
/// \brief The different math modes for the math-channel.
enum class MathMode : unsigned { ADD_CH1_CH2, SUB_CH2_FROM_CH1, SUB_CH1_FROM_CH2, MUL_CH1_CH2,
    SQ_CH1, SQ_CH2, AC_CH1, AC_CH2, DC_CH1, DC_CH2, ABS_CH1, ABS_CH2,
    SIGN_CH1, SIGN_CH2, SIGN_AC_CH1, SIGN_AC_CH2, TRIG_CH1, TRIG_CH2
};
// this "extern" declaration must match the Enum definition in "postprocessingsettings.cpp"
extern Enum< Dso::MathMode, Dso::MathMode::ADD_CH1_CH2, Dso::MathMode::TRIG_CH2 > MathModeEnum;

const auto LastBinaryMathMode = MathMode::MUL_CH1_CH2;
const auto LastMathMode = MathMode::TRIG_CH2;

template < class T > inline MathMode getMathMode( T &t ) { return MathMode( t.couplingOrMathIndex ); }

/// \enum WindowFunction
/// \brief The supported window functions.
/// These are needed for spectrum analysis and are applied to the sample values
/// before calculating the DFT.
enum class WindowFunction : int {
    RECTANGULAR,  ///< Rectangular window (aka Dirichlet)
    HAMMING,      ///< Hamming window
    HANN,         ///< Hann window
    COSINE,       ///< Cosine window (aka Sine)
    LANCZOS,      ///< Lanczos window (aka Sinc)
    BARTLETT,     ///< Bartlett window (Endpoints == 0)
    TRIANGULAR,   ///< Triangular window (Endpoints != 0)
    GAUSS,        ///< Gauss window (simga = 0.4)
    BARTLETTHANN, ///< Bartlett-Hann window
    BLACKMAN,     ///< Blackman window (alpha = 0.16)
    // KAISER,                      ///< Kaiser window (alpha = 3.0)
    NUTTALL,         ///< Nuttall window, cont. first deriv.
    BLACKMANHARRIS,  ///< Blackman-Harris window
    BLACKMANNUTTALL, ///< Blackman-Nuttall window
    FLATTOP          ///< Flat top window
};
// this "extern" declaration must match the Enum definition in "postprocessingsettings.cpp"
extern Enum< Dso::WindowFunction, Dso::WindowFunction::RECTANGULAR, Dso::WindowFunction::FLATTOP > WindowFunctionEnum;

const auto LastWindowFunction = WindowFunction::FLATTOP;

Unit mathModeUnit( MathMode mode );

QString mathModeString( MathMode mode );
// QString windowFunctionString(WindowFunction window);

} // namespace Dso

Q_DECLARE_METATYPE( Dso::MathMode )
Q_DECLARE_METATYPE( Dso::WindowFunction )

struct DsoSettingsPostProcessing {
    Dso::WindowFunction spectrumWindow = Dso::WindowFunction::HAMMING; ///< Window function for DFT
    double spectrumReference = 0.0;                                    ///< Reference level for spectrum in dBu
    double spectrumLimit = -60.0;                                      ///< Minimum magnitude of the spectrum (Avoids peaks)
};
