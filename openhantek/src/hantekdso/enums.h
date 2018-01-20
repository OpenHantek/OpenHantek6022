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
    HARDWARE_SOFTWARE, ///< Normal hardware trigger (or software trigger) mode
    WAIT_FORCE,        ///< Automatic without trigger event
    SINGLE             ///< Stop after the first trigger event
};
extern Enum<Dso::TriggerMode, Dso::TriggerMode::HARDWARE_SOFTWARE, Dso::TriggerMode::SINGLE> TriggerModeEnum;

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

QString channelModeString(ChannelMode mode);
QString graphFormatString(GraphFormat format);
QString couplingString(Coupling coupling);
QString triggerModeString(TriggerMode mode);
QString slopeString(Slope slope);
QString interpolationModeString(InterpolationMode interpolation);
}

Q_DECLARE_METATYPE(Dso::TriggerMode)
Q_DECLARE_METATYPE(Dso::Slope)
Q_DECLARE_METATYPE(Dso::Coupling)
Q_DECLARE_METATYPE(Dso::GraphFormat)
Q_DECLARE_METATYPE(Dso::ChannelMode)
Q_DECLARE_METATYPE(Dso::InterpolationMode)
