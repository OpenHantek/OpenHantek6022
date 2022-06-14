// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "utils/enumclass.h"
#include <QMetaType>
#include <QString>
namespace Dso {
/// \enum ChannelMode
/// \brief The channel display modes.
enum class ChannelMode {
    Voltage, ///< Standard voltage view
    Spectrum ///< Spectrum view
};

/// \enum GraphFormat
/// \brief The possible viewing formats for the graphs on the scope.
enum GraphFormat {
    TY, ///< The standard mode
    XY  ///< CH1 on X-axis, CH2 on Y-axis
};

extern Enum< Dso::GraphFormat, Dso::GraphFormat::TY, Dso::GraphFormat::XY > GraphFormatEnum;

/// \enum Coupling
/// \brief The coupling modes for the channels.
enum class Coupling {
    DC, ///< No filtering
    AC, ///< Offset filtered out by capacitor
    GND ///< Channel is grounded
};

/// \enum TriggerMode
/// \brief The different triggering modes.
enum class TriggerMode {
    AUTO,   ///< Automatic without trigger event
    NORMAL, ///< Normal hardware trigger (or software trigger) mode
    SINGLE, ///< Stop after the first trigger event
    ROLL    ///< Free running without any trigger
};          // <class T, T first, T last>
extern Enum< Dso::TriggerMode, Dso::TriggerMode::AUTO, Dso::TriggerMode::ROLL > TriggerModeEnum;

/// \enum Slope
/// \brief The slope that causes a trigger.
enum class Slope : uint8_t {
    Positive = 0, ///< From lower to higher voltage
    Negative = 1, ///< From higher to lower voltage
    Both = 2      ///< At 1st level crossing up or down
};
extern Enum< Dso::Slope, Dso::Slope::Positive, Dso::Slope::Both > SlopeEnum;

/// \enum InterpolationMode
/// \brief The different interpolation modes for the graphs.
enum InterpolationMode {
    INTERPOLATION_OFF = 0, ///< Just dots for each sample
    INTERPOLATION_LINEAR,  ///< Sample dots connected by straight lines
    INTERPOLATION_STEP,    ///< Sample dots connected by one step
    INTERPOLATION_SINC,    ///< Sample dots upsampled by bandlimited sinc
    INTERPOLATION_COUNT    ///< Total number of interpolation modes
};

/// \enum Themes
/// \brief The different themes for display.
enum Themes {
    THEME_AUTO = 0, ///< Use the system theme
    THEME_LIGHT,    ///< Force a light theme
    THEME_DARK      ///< Force a dark theme
};

// QString channelModeString(ChannelMode mode);
QString graphFormatString( GraphFormat format );
QString couplingString( Coupling coupling );
QString triggerModeString( TriggerMode mode );
QString slopeString( Slope slope );
// QString interpolationModeString(InterpolationMode interpolation);
} // namespace Dso

Q_DECLARE_METATYPE( Dso::TriggerMode )
Q_DECLARE_METATYPE( Dso::Slope )
Q_DECLARE_METATYPE( Dso::Coupling )
Q_DECLARE_METATYPE( Dso::GraphFormat )
Q_DECLARE_METATYPE( Dso::ChannelMode )
Q_DECLARE_METATYPE( Dso::InterpolationMode )
