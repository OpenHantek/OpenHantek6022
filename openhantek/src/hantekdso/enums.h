#pragma once

#include "utils/enumclass.h"
#include <QMetaType>
namespace Dso {
/// \enum ChannelMode
/// \brief The channel display modes.
enum class ChannelMode {
    Voltage,  ///< Standard voltage view
    Spectrum ///< Spectrum view
};
constexpr int ChannelModes = 2;
extern Enum<Dso::ChannelMode, Dso::ChannelMode::Voltage, Dso::ChannelMode::Spectrum> ChannelModeEnum;

/// \enum GraphFormat
/// \brief The possible viewing formats for the graphs on the scope.
enum GraphFormat {
    TY,   ///< The standard mode
    XY   ///< CH1 on X-axis, CH2 on Y-axis
};

extern Enum<Dso::GraphFormat, Dso::GraphFormat::TY, Dso::GraphFormat::XY> GraphFormatEnum;

/// \enum Coupling
/// \brief The coupling modes for the channels.
enum class Coupling {
    AC, ///< Offset filtered out by condensator
    DC, ///< No filtering
    GND ///< Channel is grounded
};

/// \enum TriggerMode
/// \brief The different triggering modes.
enum class TriggerMode {
    AUTO,    ///< Automatic without trigger event
    NORMAL,  ///< Normal mode
    SINGLE,  ///< Stop after the first trigger event
    SOFTWARE ///< Software trigger mode
};
extern Enum<Dso::TriggerMode, Dso::TriggerMode::AUTO, Dso::TriggerMode::SOFTWARE> TriggerModeEnum;

/// \enum Slope
/// \brief The slope that causes a trigger.
enum class Slope : uint8_t {
    Positive = 0, ///< From lower to higher voltage
    Negative = 1  ///< From higher to lower voltage
};
extern Enum<Dso::Slope, Dso::Slope::Positive, Dso::Slope::Negative> SlopeEnum;

/// \enum InterpolationMode
/// \brief The different interpolation modes for the graphs.
enum InterpolationMode {
    INTERPOLATION_OFF = 0, ///< Just dots for each sample
    INTERPOLATION_LINEAR,  ///< Sample dots connected by lines
    INTERPOLATION_SINC,    ///< Smooth graph through the dots
    INTERPOLATION_COUNT    ///< Total number of interpolation modes
};
}

Q_DECLARE_METATYPE(Dso::TriggerMode)
Q_DECLARE_METATYPE(Dso::Slope)
Q_DECLARE_METATYPE(Dso::Coupling)
Q_DECLARE_METATYPE(Dso::GraphFormat)
Q_DECLARE_METATYPE(Dso::ChannelMode)
Q_DECLARE_METATYPE(Dso::InterpolationMode)
