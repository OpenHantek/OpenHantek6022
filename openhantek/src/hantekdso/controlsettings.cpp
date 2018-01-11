#include "controlsettings.h"
#include "hantekprotocol/definitions.h"

namespace Dso {

ControlSettings::ControlSettings(ControlSamplerateLimits* limits, size_t channelCount)
{
    samplerate.limits = limits;
    trigger.level.resize(channelCount);
    voltage.resize(channelCount);
    offsetLimit = new Hantek::OffsetsPerGainStep[channelCount];
}

ControlSettings::~ControlSettings()
{
    delete [] offsetLimit;
}

}
