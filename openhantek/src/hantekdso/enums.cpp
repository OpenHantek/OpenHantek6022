#include "enums.h"
#include <QCoreApplication>

namespace Dso {
    Enum<Dso::TriggerMode, Dso::TriggerMode::HARDWARE_SOFTWARE, Dso::TriggerMode::SINGLE> TriggerModeEnum;
    Enum<Dso::Slope, Dso::Slope::Positive, Dso::Slope::Negative> SlopeEnum;
    Enum<Dso::GraphFormat, Dso::GraphFormat::TY, Dso::GraphFormat::XY> GraphFormatEnum;

    /// \brief Return string representation of the given channel mode.
    /// \param mode The ::ChannelMode that should be returned as string.
    /// \return The string that should be used in labels etc., empty when invalid.
    QString channelModeString(ChannelMode mode) {
        switch (mode) {
        case ChannelMode::Voltage:
            return QCoreApplication::tr("Voltage");
        case ChannelMode::Spectrum:
            return QCoreApplication::tr("Spectrum");
        }
        return QString();
    }

    /// \brief Return string representation of the given graph format.
    /// \param format The ::GraphFormat that should be returned as string.
    /// \return The string that should be used in labels etc.
    QString graphFormatString(GraphFormat format) {
        switch (format) {
        case GraphFormat::TY:
            return QCoreApplication::tr("T - Y");
        case GraphFormat::XY:
            return QCoreApplication::tr("X - Y");
        }
        return QString();
    }

    /// \brief Return string representation of the given channel coupling.
    /// \param coupling The ::Coupling that should be returned as string.
    /// \return The string that should be used in labels etc.
    QString couplingString(Coupling coupling) {
        switch (coupling) {
        case Coupling::AC:
            return QCoreApplication::tr("AC");
        case Coupling::DC:
            return QCoreApplication::tr("DC");
        case Coupling::GND:
            return QCoreApplication::tr("GND");
        }
        return QString();
    }


    /// \brief Return string representation of the given trigger mode.
    /// \param mode The ::TriggerMode that should be returned as string.
    /// \return The string that should be used in labels etc.
    QString triggerModeString(TriggerMode mode) {
        switch (mode) {
        case TriggerMode::WAIT_FORCE:
            return QCoreApplication::tr("Wait/Force");
        case TriggerMode::HARDWARE_SOFTWARE:
            return QCoreApplication::tr("Hard-/Software");
        case TriggerMode::SINGLE:
            return QCoreApplication::tr("Single");
        }
        return QString();
    }

    /// \brief Return string representation of the given trigger slope.
    /// \param slope The ::Slope that should be returned as string.
    /// \return The string that should be used in labels etc.
    QString slopeString(Slope slope) {
        switch (slope) {
        case Slope::Positive:
            return QString::fromUtf8("\u2197");
        case Slope::Negative:
            return QString::fromUtf8("\u2198");
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
            return QCoreApplication::tr("Off");
        case INTERPOLATION_LINEAR:
            return QCoreApplication::tr("Linear");
        case INTERPOLATION_SINC:
            return QCoreApplication::tr("Sinc");
        default:
            return QString();
        }
    }
}
