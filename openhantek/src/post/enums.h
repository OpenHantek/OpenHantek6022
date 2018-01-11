#pragma once

#include <QMetaType>
#include "utils/enumclass.h"
namespace Dso {

/// \enum MathMode
/// \brief The different math modes for the math-channel.
enum class MathMode {
    ADD_CH1_CH2,
    SUB_CH2_FROM_CH1,
    SUB_CH1_FROM_CH2
};
extern Enum<Dso::MathMode,Dso::MathMode::ADD_CH1_CH2,Dso::MathMode::SUB_CH1_FROM_CH2> MathModeEnum;

/// \enum WindowFunction
/// \brief The supported window functions.
/// These are needed for spectrum analysis and are applied to the sample values
/// before calculating the DFT.
enum class WindowFunction: int {
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
    FLATTOP         ///< Flat top window
};
extern Enum<Dso::WindowFunction,Dso::WindowFunction::RECTANGULAR,Dso::WindowFunction::FLATTOP> WindowFunctionEnum;

}

Q_DECLARE_METATYPE(Dso::MathMode)
Q_DECLARE_METATYPE(Dso::WindowFunction)

