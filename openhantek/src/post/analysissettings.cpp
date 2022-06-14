// SPDX-License-Identifier: GPL-2.0-or-later

#include "analysissettings.h"

#include <QCoreApplication>
#include <QString>

namespace Dso {

// Enum definition must match the "extern" declarations in "analysissettings.h"
Enum< Dso::WindowFunction, Dso::WindowFunction::RECTANGULAR, Dso::WindowFunction::FLATTOP > WindowFunctionEnum;

/// \brief Return string representation of the given window function.
/// \param windowFunction The ::WindowFunction that should be returned as string.
/// \return The string that should be used in labels etc.
QString windowFunctionString( WindowFunction windowFunction ) {
    switch ( windowFunction ) {
    case Dso::WindowFunction::RECTANGULAR:
        return QCoreApplication::tr( "Rectangular" );
    case Dso::WindowFunction::HANN:
        return QCoreApplication::tr( "Hann" );
    case Dso::WindowFunction::HAMMING:
        return QCoreApplication::tr( "Hamming" );
    case Dso::WindowFunction::COSINE:
        return QCoreApplication::tr( "Cosine" );
    case Dso::WindowFunction::LANCZOS:
        return QCoreApplication::tr( "Lanczos" );
    case Dso::WindowFunction::TRIANGULAR:
        return QCoreApplication::tr( "Triangular" );
    case Dso::WindowFunction::BARTLETT:
        return QCoreApplication::tr( "Bartlett" );
    case Dso::WindowFunction::BARTLETT_HANN:
        return QCoreApplication::tr( "Bartlett-Hann" );
    case Dso::WindowFunction::GAUSS:
        return QCoreApplication::tr( "Gauss" );
    case Dso::WindowFunction::KAISER:
        return QCoreApplication::tr( "Kaiser" );
    case Dso::WindowFunction::BLACKMAN:
        return QCoreApplication::tr( "Blackman" );
    case Dso::WindowFunction::NUTTALL:
        return QCoreApplication::tr( "Nuttall" );
    case Dso::WindowFunction::BLACKMAN_HARRIS:
        return QCoreApplication::tr( "Blackman-Harris" );
    case Dso::WindowFunction::BLACKMAN_NUTTALL:
        return QCoreApplication::tr( "Blackman-Nuttall" );
    case Dso::WindowFunction::FLATTOP:
        return QCoreApplication::tr( "Flat top" );
    }
    return QString();
}

} // namespace Dso
