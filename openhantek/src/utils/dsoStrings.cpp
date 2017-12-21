////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dso.cpp
//
//  Copyright (C) 2010  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <QApplication>

#include "dsoStrings.h"

namespace Dso {
/// \brief Return string representation of the given channel mode.
/// \param mode The ::ChannelMode that should be returned as string.
/// \return The string that should be used in labels etc., empty when invalid.
QString channelModeString(ChannelMode mode) {
    switch (mode) {
    case CHANNELMODE_VOLTAGE:
        return QApplication::tr("Voltage");
    case CHANNELMODE_SPECTRUM:
        return QApplication::tr("Spectrum");
    default:
        return QString();
    }
}

/// \brief Return string representation of the given graph format.
/// \param format The ::GraphFormat that should be returned as string.
/// \return The string that should be used in labels etc.
QString graphFormatString(GraphFormat format) {
    switch (format) {
    case GRAPHFORMAT_TY:
        return QApplication::tr("T - Y");
    case GRAPHFORMAT_XY:
        return QApplication::tr("X - Y");
    default:
        return QString();
    }
}

/// \brief Return string representation of the given channel coupling.
/// \param coupling The ::Coupling that should be returned as string.
/// \return The string that should be used in labels etc.
QString couplingString(Coupling coupling) {
    switch (coupling) {
    case COUPLING_AC:
        return QApplication::tr("AC");
    case COUPLING_DC:
        return QApplication::tr("DC");
    case COUPLING_GND:
        return QApplication::tr("GND");
    default:
        return QString();
    }
}

/// \brief Return string representation of the given math mode.
/// \param mode The ::MathMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString mathModeString(MathMode mode) {
    switch (mode) {
    case MathMode::ADD_CH1_CH2:
        return QApplication::tr("CH1 + CH2");
    case MathMode::SUB_CH2_FROM_CH1:
        return QApplication::tr("CH1 - CH2");
    case MathMode::SUB_CH1_FROM_CH2:
        return QApplication::tr("CH2 - CH1");
    default:
        return QString();
    }
}

/// \brief Return string representation of the given trigger mode.
/// \param mode The ::TriggerMode that should be returned as string.
/// \return The string that should be used in labels etc.
QString triggerModeString(TriggerMode mode) {
    switch (mode) {
    case TRIGGERMODE_AUTO:
        return QApplication::tr("Auto");
    case TRIGGERMODE_NORMAL:
        return QApplication::tr("Normal");
    case TRIGGERMODE_SINGLE:
        return QApplication::tr("Single");
    case TRIGGERMODE_SOFTWARE:
        return QApplication::tr("Software");
    default:
        return QString();
    }
}

/// \brief Return string representation of the given trigger slope.
/// \param slope The ::Slope that should be returned as string.
/// \return The string that should be used in labels etc.
QString slopeString(Slope slope) {
    switch (slope) {
    case SLOPE_POSITIVE:
        return QString::fromUtf8("\u2197");
    case SLOPE_NEGATIVE:
        return QString::fromUtf8("\u2198");
    default:
        return QString();
    }
}

/// \brief Return string representation of the given dft window function.
/// \param window The ::WindowFunction that should be returned as string.
/// \return The string that should be used in labels etc.
QString windowFunctionString(WindowFunction window) {
    switch (window) {
    case WindowFunction::RECTANGULAR:
        return QApplication::tr("Rectangular");
    case WindowFunction::HAMMING:
        return QApplication::tr("Hamming");
    case WindowFunction::HANN:
        return QApplication::tr("Hann");
    case WindowFunction::COSINE:
        return QApplication::tr("Cosine");
    case WindowFunction::LANCZOS:
        return QApplication::tr("Lanczos");
    case WindowFunction::BARTLETT:
        return QApplication::tr("Bartlett");
    case WindowFunction::TRIANGULAR:
        return QApplication::tr("Triangular");
    case WindowFunction::GAUSS:
        return QApplication::tr("Gauss");
    case WindowFunction::BARTLETTHANN:
        return QApplication::tr("Bartlett-Hann");
    case WindowFunction::BLACKMAN:
        return QApplication::tr("Blackman");
    // case WindowFunction::WINDOW_KAISER:
    //	return QApplication::tr("Kaiser");
    case WindowFunction::NUTTALL:
        return QApplication::tr("Nuttall");
    case WindowFunction::BLACKMANHARRIS:
        return QApplication::tr("Blackman-Harris");
    case WindowFunction::BLACKMANNUTTALL:
        return QApplication::tr("Blackman-Nuttall");
    case WindowFunction::FLATTOP:
        return QApplication::tr("Flat top");
    default:
        return QString();
    }
}

/// \brief Return string representation of the given graph interpolation mode.
/// \param interpolation The ::InterpolationMode that should be returned as
/// string.
/// \return The string that should be used in labels etc.
QString interpolationModeString(InterpolationMode interpolation) {
    switch (interpolation) {
    case INTERPOLATION_OFF:
        return QApplication::tr("Off");
    case INTERPOLATION_LINEAR:
        return QApplication::tr("Linear");
    case INTERPOLATION_SINC:
        return QApplication::tr("Sinc");
    default:
        return QString();
    }
}
}
