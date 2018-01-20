#include "postprocessingsettings.h"

#include <QCoreApplication>
#include <QString>

namespace Dso {

Enum<Dso::MathMode, Dso::MathMode::ADD_CH1_CH2, Dso::MathMode::SUB_CH1_FROM_CH2> MathModeEnum;
Enum<Dso::WindowFunction, Dso::WindowFunction::RECTANGULAR, Dso::WindowFunction::FLATTOP> WindowFunctionEnum;

/// \brief Return string representation of the given math mode.
/// \param mode The ::MathMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString mathModeString(MathMode mode) {
    switch (mode) {
    case MathMode::ADD_CH1_CH2:
        return QCoreApplication::tr("CH1 + CH2");
    case MathMode::SUB_CH2_FROM_CH1:
        return QCoreApplication::tr("CH1 - CH2");
    case MathMode::SUB_CH1_FROM_CH2:
        return QCoreApplication::tr("CH2 - CH1");
    }
    return QString();
}
/// \brief Return string representation of the given dft window function.
/// \param window The ::WindowFunction that should be returned as string.
/// \return The string that should be used in labels etc.
QString windowFunctionString(WindowFunction window) {
    switch (window) {
    case WindowFunction::RECTANGULAR:
        return QCoreApplication::tr("Rectangular");
    case WindowFunction::HAMMING:
        return QCoreApplication::tr("Hamming");
    case WindowFunction::HANN:
        return QCoreApplication::tr("Hann");
    case WindowFunction::COSINE:
        return QCoreApplication::tr("Cosine");
    case WindowFunction::LANCZOS:
        return QCoreApplication::tr("Lanczos");
    case WindowFunction::BARTLETT:
        return QCoreApplication::tr("Bartlett");
    case WindowFunction::TRIANGULAR:
        return QCoreApplication::tr("Triangular");
    case WindowFunction::GAUSS:
        return QCoreApplication::tr("Gauss");
    case WindowFunction::BARTLETTHANN:
        return QCoreApplication::tr("Bartlett-Hann");
    case WindowFunction::BLACKMAN:
        return QCoreApplication::tr("Blackman");
    // case WindowFunction::WINDOW_KAISER:
    //	return QCoreApplication::tr("Kaiser");
    case WindowFunction::NUTTALL:
        return QCoreApplication::tr("Nuttall");
    case WindowFunction::BLACKMANHARRIS:
        return QCoreApplication::tr("Blackman-Harris");
    case WindowFunction::BLACKMANNUTTALL:
        return QCoreApplication::tr("Blackman-Nuttall");
    case WindowFunction::FLATTOP:
        return QCoreApplication::tr("Flat top");
    }
    return QString();
}
}
