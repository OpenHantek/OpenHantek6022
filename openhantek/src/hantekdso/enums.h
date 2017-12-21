#pragma once

#include <QMetaType>
#include "utils/enumclass.h"
namespace Dso {
/// \enum ChannelMode
/// \brief The channel display modes.
enum ChannelMode {
    CHANNELMODE_VOLTAGE,  ///< Standard voltage view
    CHANNELMODE_SPECTRUM, ///< Spectrum view
    CHANNELMODE_COUNT     ///< The total number of modes
};

/// \enum GraphFormat
/// \brief The possible viewing formats for the graphs on the scope.
enum GraphFormat {
    GRAPHFORMAT_TY,   ///< The standard mode
    GRAPHFORMAT_XY,   ///< CH1 on X-axis, CH2 on Y-axis
    GRAPHFORMAT_COUNT ///< The total number of formats
};

/// \enum Coupling
/// \brief The coupling modes for the channels.
enum Coupling {
    COUPLING_AC,   ///< Offset filtered out by condensator
    COUPLING_DC,   ///< No filtering
    COUPLING_GND,  ///< Channel is grounded
    COUPLING_COUNT ///< The total number of coupling modes
};

/// \enum TriggerMode
/// \brief The different triggering modes.
enum TriggerMode {
    TRIGGERMODE_AUTO,     ///< Automatic without trigger event
    TRIGGERMODE_NORMAL,   ///< Normal mode
    TRIGGERMODE_SINGLE,   ///< Stop after the first trigger event
    TRIGGERMODE_SOFTWARE, ///< Software trigger mode
    TRIGGERMODE_COUNT     ///< The total number of modes
};

/// \enum Slope
/// \brief The slope that causes a trigger.
enum Slope {
    SLOPE_POSITIVE, ///< From lower to higher voltage
    SLOPE_NEGATIVE, ///< From higher to lower voltage
    SLOPE_COUNT     ///< Total number of trigger slopes
};

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

