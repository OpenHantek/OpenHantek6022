// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "utils/enumclass.h"

#include <QMetaType>

namespace Dso {

/// \enum WindowFunction
/// \brief The supported window functions.
/// These are needed for spectrum analysis and are applied to the sample values
/// before calculating the DFT.
enum class WindowFunction : int {
    RECTANGULAR,      ///< Rectangular window (aka Dirichlet)
    HANN,             ///< Hann window
    HAMMING,          ///< Hamming window
    COSINE,           ///< Cosine window (aka Sine)
    LANCZOS,          ///< Lanczos window (aka Sinc)
    TRIANGULAR,       ///< Triangular window (Endpoints != 0)
    BARTLETT,         ///< Bartlett window (Endpoints == 0)
    BARTLETT_HANN,    ///< Bartlett-Hann window
    GAUSS,            ///< Gauss window (sigma = 0.3)
    KAISER,           ///< Kaiser window (alpha = 3.0)
    BLACKMAN,         ///< Blackman window (alpha = 0.16)
    NUTTALL,          ///< Nuttall window, cont. first deriv.
    BLACKMAN_HARRIS,  ///< Blackman-Harris window
    BLACKMAN_NUTTALL, ///< Blackman-Nuttall window
    FLATTOP           ///< Flat top window
};
// this "extern" declaration must match the Enum definition in "analysissettings.cpp"
extern Enum< Dso::WindowFunction, Dso::WindowFunction::RECTANGULAR, Dso::WindowFunction::FLATTOP > WindowFunctionEnum;

const auto LastWindowFunction = WindowFunction::FLATTOP;

QString windowFunctionString( WindowFunction window );

} // namespace Dso

Q_DECLARE_METATYPE( Dso::WindowFunction )

struct DsoSettingsAnalysis {
    Dso::WindowFunction spectrumWindow = Dso::WindowFunction::HAMMING; ///< Window function for DFT
    double spectrumLimit = -60.0;                                      ///< Minimum magnitude of the spectrum (Avoids peaks)
    bool reuseFftPlan = false;                                         ///< Optimize FFT plan and reuse it
};
