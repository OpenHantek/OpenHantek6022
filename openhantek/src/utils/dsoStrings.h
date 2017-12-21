// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include <QString>
#include "analyse/enums.h"
#include "hantekdso/enums.h"
#include "definitions.h"

#define MARKER_COUNT 2 ///< Number of markers

/// \namespace Dso
/// \brief All DSO specific things for different modes and so on.
namespace Dso {

QString channelModeString(ChannelMode mode);
QString graphFormatString(GraphFormat format);
QString couplingString(Coupling coupling);
QString mathModeString(MathMode mode);
QString triggerModeString(TriggerMode mode);
QString slopeString(Slope slope);
QString windowFunctionString(WindowFunction window);
QString interpolationModeString(InterpolationMode interpolation);
}
