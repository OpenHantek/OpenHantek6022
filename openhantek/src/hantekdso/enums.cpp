#include "enums.h"

namespace Dso {
    Enum<Dso::TriggerMode, Dso::TriggerMode::HARDWARE_SOFTWARE, Dso::TriggerMode::SINGLE> TriggerModeEnum;
    Enum<Dso::Slope, Dso::Slope::Positive, Dso::Slope::Negative> SlopeEnum;
    Enum<Dso::GraphFormat, Dso::GraphFormat::TY, Dso::GraphFormat::XY> GraphFormatEnum;
    Enum<Dso::ChannelMode, Dso::ChannelMode::Voltage, Dso::ChannelMode::Spectrum> ChannelModeEnum;
}
